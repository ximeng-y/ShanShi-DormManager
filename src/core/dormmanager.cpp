#include "dormmanager.h"
#include "studentmanager.h"
#include "system/check.h"

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

	//删除前记录并清空住客，随后同步清理 student 本体位置字段
	QVector<int> student_ids = dorms[building_id][dorm_id].get_student_id_list();
	dorms[building_id][dorm_id].clear_students();
	for (int student_id : student_ids)
		studentmanager::instance().clear_dorm_info(student_id);

	//从QMap<int, dorm>中移除宿舍对象，同步清理宿舍对象的内存空间
	dorms[building_id].remove(dorm_id);//移除宿舍对象
	if (dorms[building_id].isEmpty())//如果楼内没有宿舍了
		dorms.remove(building_id);//移除楼对象
	return true;
}

//设置房间性别锁，只处理宿舍集合定位与 dorm 自身字段写入
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

	if (!dorm_it.value().set_for_gender(gender))
		return -1;//理论仅可能来自 gender 非法，前置已阻挡
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

int dormmanager::add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender)//向指定宿舍写入学生学号
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id) || (gender != 1 && gender != 2))
		return -1;//参数非法
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return -8;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return -8;//宿舍不存在
	return dorm_it.value().add_student(student_id, gender);
}

int dormmanager::add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender, int bed_id)//向指定床位写入学生学号
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id) || (gender != 1 && gender != 2))
		return -1;//参数非法
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return -8;//宿舍不存在
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return -8;//宿舍不存在
	if (!check::is_valid_bed_id(bed_id, dorm_it.value().get_max_num()))
		return -1;//bed_id非法
	return dorm_it.value().add_student(student_id, gender, bed_id);
}

int dormmanager::remove_student_from_dorm(int building_id, int dorm_id, int student_id)//从指定宿舍移除学生学号
{
	if (!check::is_valid_building_id(building_id) || !check::is_valid_dorm_id(dorm_id) || !check::is_valid_student_id(student_id))
		return -1;//参数非法
	dorm* target = find_dorm(building_id, dorm_id);
	if (target == nullptr)
		return -8;//宿舍不存在
	return target->remove_student(student_id);
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

QVector<QPair<int, int>> dormmanager::all_dorm_keys() const//按楼号、宿舍号升序列出全部宿舍复合键
{
	QVector<QPair<int, int>> keys;
	for (auto b_it = dorms.constBegin(); b_it != dorms.constEnd(); ++b_it)
		for (auto d_it = b_it.value().constBegin(); d_it != b_it.value().constEnd(); ++d_it)
			keys.append(qMakePair(b_it.key(), d_it.key()));
	return keys;
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

int dormmanager::clear_dorm_students(int building_id, int dorm_id, bool reset_gender)//清空指定宿舍住客
{
	dorm* target = find_dorm(building_id, dorm_id);
	if (target == nullptr)
		return -8;
	int cleared = target->get_current_num();
	if (reset_gender)
		target->clear_students_reset_gender();
	else
		target->clear_students();
	return cleared;
}
