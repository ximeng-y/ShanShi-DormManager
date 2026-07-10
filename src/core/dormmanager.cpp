#include "dormmanager.h"
#include "buildingmanager.h"
#include "studentmanager.h"
#include "system/check.h"
#include <QRandomGenerator>

dormmanager& dormmanager::instance()
{
	static dormmanager mgr;
	return mgr;
}

//按楼号、宿舍号取本体（只读）
const dorm* dormmanager::get(int building_id, int dorm_id) const
{
	auto building_it = dorms.constFind(building_id);
	if (building_it == dorms.constEnd())//先判断楼号是否存在
		return nullptr;
	auto dorm_it = building_it->constFind(dorm_id);
	if (dorm_it == building_it->constEnd())//再判断宿舍号是否存在
		return nullptr;
	return &dorm_it.value();
}

//检查指定楼号、宿舍号是否存在
int dormmanager::is_dorm_exist(int building_id, int dorm_id)
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id))
		return -1;//参数非法
	auto building_it = dorms.constFind(building_id);
	if (building_it == dorms.constEnd() || !building_it->contains(dorm_id))
		return 0;//不存在
	return 1;//存在
}

//获取指定性别可用的宿舍(默认最小可用楼号中的最小可用宿舍号)
//QMap 遍历天然按 building_id 升序、内层按 dorm_id 升序, 故首个命中的即最小顺位。
//匹配条件: 楼已在 buildingmanager 注册且性别接纳该 gender, 且宿舍未满。无可用返回 nullptr。
const dorm* dormmanager::get_available_dorm(int gender)
{
	//学生性别只应为 1=男 / 2=女; 0=未设置或其它非法值直接拒绝
	if (gender != 1 && gender != 2)
		return nullptr;

	for (auto b_it = dorms.constBegin(); b_it != dorms.constEnd(); ++b_it)
	{
		//横向问 buildingmanager: 这栋楼的适用性别
		const building* b = buildingmanager::instance().get(b_it.key());//获取楼信息指针
		if (b == nullptr)//楼未注册, 该楼所有宿舍视为不可用, 跳过
			continue;
		if (!b->accepts_gender(gender))//性别不接纳, 整栋跳过
			continue;

		for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
		{
			const dorm& d = d_it.value();
			if (d.accepts_gender(gender) && !d.is_full())//房间性别接纳且未满, 首个命中即最小顺位
				return &d_it.value();
		}
	}
	return nullptr;//全校无可用宿舍
}

//获取指定性别可用的宿舍(在所有可用宿舍中随机选一间)
//先收集全部满足条件(性别接纳且未满)的候选, 再用 QRandomGenerator 等概率抽取。无可用返回 nullptr。
const dorm* dormmanager::get_available_dorm_random(int gender)
{
	if (gender != 1 && gender != 2)// 学生性别只应为 1=男 / 2=女; 0=未设置或其它非法值直接拒绝
		return nullptr;

	QVector<const dorm*> candidates;//收集所有可用宿舍的指针(就地使用, 收集期间不发生增删, 指针有效)
	for (auto b_it = dorms.constBegin(); b_it != dorms.constEnd(); ++b_it)
	{
		const building* b = buildingmanager::instance().get(b_it.key());//获取楼信息指针
		if (b == nullptr)//楼未注册, 该楼所有宿舍视为不可用, 跳过	
			continue;
		if (!b->accepts_gender(gender))//性别不接纳, 整栋跳过
			continue;

		for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
		{
			const dorm& d = d_it.value();
			if (d.accepts_gender(gender) && !d.is_full())//房间性别接纳且未满
				candidates.append(&d_it.value());
		}
	}

	if (candidates.isEmpty())//无可用宿舍
		return nullptr;
	//bounded(n) 返回 [0, n) 的等概率随机数, 从候选中抽取一间宿舍指针
	return candidates[QRandomGenerator::global()->bounded(candidates.size())];//QRandomGenerator::global()：Qt 的全局随机数生成器
}

//添加宿舍（building_id 取自 dorm 内部）
bool dormmanager::add_dorm(const dorm& dorm_to_add)
{
	int building_id = dorm_to_add.get_building_id();//从dorm对象中获取楼号
	int dorm_id = dorm_to_add.get_id();//从dorm对象中获取宿舍号

	//前置校验: building_id 合法
	if (!check::is_valid_building_id(building_id))
		return false;
	//前置校验: dorm 必须已配置（id 合法且 max_num>=1）
	if (!check::is_valid_dorm_id(dorm_id))
		return false;
	if (dorm_to_add.get_max_num() < 1)
		return false;
	//唯一性校验: 同楼内 dorm_id 不得重复
	if (dorms.contains(building_id) && dorms[building_id].contains(dorm_id))
		return false;
	//楼级校验: dorm_id 派生楼层(dorm_id/100)须在所在 building 的 max_floor 内。
	//max_floor 活在 building 本体(归 buildingmanager), 故 building 必须已注册; 未注册则 dorm 不应挂到不存在的楼。
	const building* b = buildingmanager::instance().get(building_id);
	if (b == nullptr)
		return false;//楼未注册
	if (!check::is_valid_dorm_floor(dorm_id, b->get_max_floor()))
		return false;//派生楼层越界(超过楼最大楼层)

	dorms[building_id].insert(dorm_id, dorm_to_add);//将宿舍对象插入到QMap<int, dorm>中
	return true;
}

//删除宿舍（自动清空宿舍内学生）
bool dormmanager::remove_dorm(int building_id, int dorm_id)
{
	//前置校验: 参数合法性
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id))
		return false;
	//存在性校验
	if (!dorms.contains(building_id) || !dorms[building_id].contains(dorm_id))
		return false;

	//调用宿舍对象的clear_students()方法，自动清空宿舍内学生（同步清理 student 本体位置字段）
	dorms[building_id][dorm_id].clear_students();

	//从QMap<int, dorm>中移除宿舍对象，同步清理宿舍对象的内存空间
	dorms[building_id].remove(dorm_id);//移除宿舍对象
	if (dorms[building_id].isEmpty())//如果楼内没有宿舍了
		dorms.remove(building_id);//移除楼对象
	return true;
}

//钦定房间性别(经 friend 后门调 dorm::set_for_gender, 前置做楼-房一致性 G2 校验)
int dormmanager::set_dorm_gender(int building_id, int dorm_id, int gender)
{
	//参数合法性: building_id/dorm_id 格式 + gender 格式(0/1/2)
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id))
		return -1;//参数非法
	if (!check::is_valid_gender(gender))
		return -1;//gender 非法

	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return 0;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return 0;//宿舍不存在

	//钦定(非 0)时做楼-房一致性校验; 解锁(0)是放开限制, 无需楼级校验
	if (gender != 0)
	{
		const building* b = buildingmanager::instance().get(building_id);
		if (b == nullptr || !b->accepts_gender(gender))
			return -2;//楼未注册或楼适用性别不接纳该 gender(会在男生楼里造女舍死间)
	}

	//经 friend 后门调 dorm::set_for_gender; 其内部再校验住客性别一致性(gender 合法已保证, 此处失败只可能是住客不符)
	if (!dorm_it.value().set_for_gender(gender))
		return -3;//房间已有住客且性别与钦定值冲突
	return 1;//成功
}

int dormmanager::set_dorm_max_num(int building_id, int dorm_id, int max_num)//修改宿舍最大人数
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || max_num < 1)
		return -1;//参数非法
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return 0;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return 0;//宿舍不存在
	return dorm_it.value().set_max_num(max_num) ? 1 : -2;//false 只应来自缩容丢人
}

int dormmanager::add_student_to_dorm(int building_id, int dorm_id, int student_id)//入住指定宿舍
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;//参数非法
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return -8;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return -8;//宿舍不存在
	return dorm_it.value().add_student(student_id);
}

int dormmanager::add_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id)//入住指定床位
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;//参数非法
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return -8;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return -8;//宿舍不存在
	return dorm_it.value().add_student(student_id, bed_id);
}

int dormmanager::add_student_to_available_dorm(int student_id)//入住最小顺位可用宿舍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	const student* s = studentmanager::instance().get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学号未注册或学生性别未设置
	const dorm* available = get_available_dorm(s->get_gender());
	if (available == nullptr)
		return -9;//无可用宿舍

	auto building_it = dorms.find(available->get_building_id());
	if (building_it == dorms.end())
		return -9;//理论不可达: 候选来自 dorms
	auto dorm_it = building_it->find(available->get_id());
	if (dorm_it == building_it->end())
		return -9;//理论不可达: 候选来自 dorms
	return dorm_it.value().add_student(student_id);
}

int dormmanager::add_student_to_available_dorm_random(int student_id)//随机入住可用宿舍
{
	if (!check::is_valid_student_id(student_id))
		return -1;//student_id非法
	const student* s = studentmanager::instance().get(student_id);
	if (s == nullptr || s->get_gender() == 0)
		return -6;//学号未注册或学生性别未设置
	const dorm* available = get_available_dorm_random(s->get_gender());
	if (available == nullptr)
		return -9;//无可用宿舍

	auto building_it = dorms.find(available->get_building_id());
	if (building_it == dorms.end())
		return -9;//理论不可达: 候选来自 dorms
	auto dorm_it = building_it->find(available->get_id());
	if (dorm_it == building_it->end())
		return -9;//理论不可达: 候选来自 dorms
	return dorm_it.value().add_student(student_id);
}

//高级信息查询
int dormmanager::count() const//获取当前宿舍总数
{
	int total = 0;
	for (auto it = dorms.constBegin(); it != dorms.constEnd(); ++it)
		total += it.value().size();//统计每个楼的宿舍数(it.value()拿到QMap<int, dorm>，size()方法返回宿舍数)
	return total;
}

int dormmanager::get_empty_count() const//获取当前空宿舍总数
{
	int empty_count = 0;
	for (auto it = dorms.constBegin(); it != dorms.constEnd(); ++it)
	{
		for (auto dorm_it = it.value().constBegin(); dorm_it != it.value().constEnd(); ++dorm_it)
		{
			if (dorm_it.value().is_empty())//如果宿舍内没有学生(此处is_empty()是dorm类的方法)
				empty_count++;//空宿舍数增加
		}
	}
	return empty_count;
}

int dormmanager::get_occupied_count() const//获取当前已占用宿舍总数
{
	return count() - get_empty_count();//已占用宿舍数=总宿舍数-空宿舍数
}

//====== 空床位总数统计 ======
int dormmanager::get_empty_bed_count() const//全校空床位总数
{
	int total = 0;
	for (auto b_it = dorms.constBegin(); b_it != dorms.constEnd(); ++b_it)
		for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
			total += d_it.value().get_empty_count();
	return total;
}

int dormmanager::get_empty_bed_count(int gender) const//指定性别可用空床
{
	if (gender != 1 && gender != 2)
		return -1;//性别只接受 1/2

	int total = 0;
	for (auto b_it = dorms.constBegin(); b_it != dorms.constEnd(); ++b_it)
	{
		const building* b = buildingmanager::instance().get(b_it.key());
		if (b == nullptr || !b->accepts_gender(gender))
			continue;//楼未注册或整栋不接纳该性别(纯异性楼), 跳过
		for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
		{
			const dorm& d = d_it.value();
			if (d.accepts_gender(gender))//房间接纳该性别(未锁定房 accepts 任意, 符合"算入"约定)
				total += d.get_empty_count();
		}
	}
	return total;
}

int dormmanager::get_empty_bed_count_of_building(int building_id) const//指定楼空床位总数
{
	if (!check::is_valid_building_id(building_id))
		return -1;//参数非法
	auto b_it = dorms.constFind(building_id);
	if (b_it == dorms.constEnd())
		return 0;//该楼在 dormmanager 中无宿舍, 空床为0
	int total = 0;
	for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
		total += d_it.value().get_empty_count();
	return total;
}

int dormmanager::get_empty_bed_count_of_building(int building_id, int gender) const//指定楼+性别
{
	if (!check::is_valid_building_id(building_id) || (gender != 1 && gender != 2))
		return -1;//参数非法
	const building* b = buildingmanager::instance().get(building_id);
	if (b == nullptr || !b->accepts_gender(gender))
		return 0;//楼未注册或不接纳该性别, 该性别空床为0
	auto b_it = dorms.constFind(building_id);
	if (b_it == dorms.constEnd())
		return 0;//该楼无宿舍
	int total = 0;
	for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
	{
		const dorm& d = d_it.value();
		if (d.accepts_gender(gender))
			total += d.get_empty_count();
	}
	return total;
}

//====== 清空所有宿舍 ======
int dormmanager::clear_all_dorms()//保留各房间性别锁
{
	int cleared = 0;
	for (auto b_it = dorms.begin(); b_it != dorms.end(); ++b_it)
		for (auto d_it = b_it.value().begin(); d_it != b_it.value().end(); ++d_it)
		{
			cleared += d_it.value().get_current_num();//先累加本间人数
			d_it.value().clear_students();//再清空(内部同步清零学生位置字段)
		}
	return cleared;
}

int dormmanager::clear_all_dorms_reset_gender()//清空并放开所有房间性别锁
{
	int cleared = 0;
	for (auto b_it = dorms.begin(); b_it != dorms.end(); ++b_it)
		for (auto d_it = b_it.value().begin(); d_it != b_it.value().end(); ++d_it)
		{
			cleared += d_it.value().get_current_num();
			d_it.value().clear_students_reset_gender();
		}
	return cleared;
}

//内部定位: 返回可写 dorm 指针(不存在返回 nullptr)。指针指向 QMap 内部,
//只要在使用期间不对 dorms 做 insert/remove, 指针始终有效(QMap 红黑树, 就地改值不失效)。
dorm* dormmanager::find_dorm(int building_id, int dorm_id)
{
	auto b_it = dorms.find(building_id);
	if (b_it == dorms.end())
		return nullptr;
	auto d_it = b_it.value().find(dorm_id);
	if (d_it == b_it.value().end())
		return nullptr;
	return &d_it.value();
}

//====== 任务1: 整体调换(性别锁相同 + 人数相同) ======
int dormmanager::swap_dorms(int b1, int d1, int b2, int d2)
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) ||
		!check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2))
		return -1;//参数非法
	if (b1 == b2 && d1 == d2)
		return -1;//指向同一间, 无意义

	dorm* A = find_dorm(b1, d1);
	dorm* B = find_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;//某间不存在

	if (A->get_for_gender() != B->get_for_gender())
		return -3;//性别锁不同, 无法整体调换
	if (A->get_current_num() != B->get_current_num())
		return -4;//性别相同但人数不同(留给顶层选择善后策略)

	//先抓两边住客名单, 再清空两边(保留性别锁), 最后交叉入住。
	//人数相同 → 对方容量天然够(A本就装着与B等量的人), add_student 不会满员。
	//性别锁相同 → 保留锁定不影响异侧入住(都是同性别)。
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	A->clear_students();
	B->clear_students();
	for (int id : listA)
		B->add_student(id);//A 的人搬进 B
	for (int id : listB)
		A->add_student(id);//B 的人搬进 A
	return 1;
}

//====== 任务1 善后B: 重叠床位互换, 多余留原地 ======
int dormmanager::swap_dorms_overlap(int b1, int d1, int b2, int d2)
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) ||
		!check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2))
		return -1;
	if (b1 == b2 && d1 == d2)
		return -1;

	dorm* A = find_dorm(b1, d1);
	dorm* B = find_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	if (A->get_for_gender() != B->get_for_gender())
		return -3;//性别锁不同一定换不了

	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = (listA.size() < listB.size()) ? listA.size() : listB.size();//重叠人数
	if (k == 0)
		return 1;//一方为空, 无可互换

	//先把两边前 k 人搬出(离宿态), 再交叉入住; 人多一方 k 之后的人从未被动, 留原地。
	for (int i = 0; i < k; ++i)
	{
		A->remove_student(listA[i]);
		B->remove_student(listB[i]);
	}
	for (int i = 0; i < k; ++i)
	{
		B->add_student(listA[i]);
		A->add_student(listB[i]);
	}
	return 1;
}

//====== 任务1 善后C: 重叠床位互换, 多余离宿 ======
int dormmanager::swap_dorms_overlap_evict(int b1, int d1, int b2, int d2)
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) ||
		!check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2))
		return -1;
	if (b1 == b2 && d1 == d2)
		return -1;

	dorm* A = find_dorm(b1, d1);
	dorm* B = find_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;
	if (A->get_for_gender() != B->get_for_gender())
		return -3;

	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = (listA.size() < listB.size()) ? listA.size() : listB.size();

	//清空两边(保留性别锁), 只把前 k 对交叉入住; 人多一方 k 之后的人不再入住 → 保持离宿态。
	A->clear_students();
	B->clear_students();
	for (int i = 0; i < k; ++i)
	{
		B->add_student(listA[i]);
		A->add_student(listB[i]);
	}
	return 1;
}

//====== 任务4: 混合楼间男舍↔女舍互换 ======
int dormmanager::swap_gender_dorms(int b1, int d1, int b2, int d2)
{
	if (!check::is_valid_building_id(b1) || !check::is_valid_dorm_id(d1) ||
		!check::is_valid_building_id(b2) || !check::is_valid_dorm_id(d2))
		return -1;
	if (b1 == b2 && d1 == d2)
		return -1;

	dorm* A = find_dorm(b1, d1);
	dorm* B = find_dorm(b2, d2);
	if (A == nullptr || B == nullptr)
		return -2;

	//前提1: 两间所在楼都必须是混宿楼(for_gender==3)
	const building* bA = buildingmanager::instance().get(b1);
	const building* bB = buildingmanager::instance().get(b2);
	if (bA == nullptr || bB == nullptr || bA->get_for_gender() != 3 || bB->get_for_gender() != 3)
		return -3;//有楼未注册或非混宿楼

	//前提2: 两间恰为一男舍一女舍
	int gA = A->get_for_gender();
	int gB = B->get_for_gender();
	if (!((gA == 1 && gB == 2) || (gA == 2 && gB == 1)))
		return -4;//不是一男一女, 无从做男女互换

	//清空并放开性别锁(否则异性无法入住), 前 k 对交叉入住, 人多一方多余的人离宿。
	//add_student 先到先得会把 A 重新锁成异性、B 锁成异性。
	QVector<int> listA = A->get_student_id_list();
	QVector<int> listB = B->get_student_id_list();
	int k = (listA.size() < listB.size()) ? listA.size() : listB.size();
	A->clear_students_reset_gender();
	B->clear_students_reset_gender();
	for (int i = 0; i < k; ++i)
	{
		B->add_student(listA[i]);//原A住客搬进B
		A->add_student(listB[i]);//原B住客搬进A
	}
	return 1;
}
