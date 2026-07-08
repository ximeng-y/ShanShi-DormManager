#include "dormmanager.h"
#include "buildingmanager.h"
#include "system/check.h"
#include <QRandomGenerator>

dormmanager& dormmanager::instance()
{
	static dormmanager mgr;
	return mgr;
}

//按楼号、宿舍号取本体（可修改）
dorm* dormmanager::get(int building_id, int dorm_id)
{
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())//先判断楼号是否存在
		return nullptr;
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())//再判断宿舍号是否存在
		return nullptr;
	return &dorm_it.value();
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
	if (!dorms.contains(building_id) || !dorms[building_id].contains(dorm_id))
		return 0;//不存在
	return 1;//存在
}

//判断某楼的适用性别 for_gender 是否接纳学生性别 gender: 混宿(3)接纳任意, 否则要求相等
static bool building_accepts_gender(int for_gender, int gender)
{
	return for_gender == 3 || for_gender == gender;
}

//判断某房间的性别锁定 for_gender 是否接纳学生性别 gender: 未锁定(0)接纳任意, 否则要求相等
//单性别楼里房间锁定必与楼一致, 该谓词只在混宿楼里真正区分男舍/女舍
static bool dorm_accepts_gender(int for_gender, int gender)
{
	return for_gender == 0 || for_gender == gender;
}

//获取指定性别可用的宿舍(默认最小可用楼号中的最小可用宿舍号)
//QMap 遍历天然按 building_id 升序、内层按 dorm_id 升序, 故首个命中的即最小顺位。
//匹配条件: 楼已在 buildingmanager 注册且性别接纳该 gender, 且宿舍未满。无可用返回 nullptr。
dorm* dormmanager::get_available_dorm(int gender)
{
	//学生性别只应为 1=男 / 2=女; 0=未设置或其它非法值直接拒绝
	if (gender != 1 && gender != 2)
		return nullptr;

	for (auto b_it = dorms.begin(); b_it != dorms.end(); ++b_it)
	{
		//横向问 buildingmanager: 这栋楼的适用性别
		const building* b = buildingmanager::instance().get(b_it.key());
		if (b == nullptr)//楼未注册, 该楼所有宿舍视为不可用, 跳过
			continue;
		if (!building_accepts_gender(b->get_for_gender(), gender))//性别不接纳, 整栋跳过
			continue;

		for (auto d_it = b_it.value().begin(); d_it != b_it.value().end(); ++d_it)
		{
			const dorm& d = d_it.value();
			if (dorm_accepts_gender(d.get_for_gender(), gender) && !d.is_full())//房间性别接纳且未满, 首个命中即最小顺位
				return &d_it.value();
		}
	}
	return nullptr;//全校无可用宿舍
}

//获取指定性别可用的宿舍(在所有可用宿舍中随机选一间)
//先收集全部满足条件(性别接纳且未满)的候选, 再用 QRandomGenerator 等概率抽取。无可用返回 nullptr。
dorm* dormmanager::get_available_dorm_random(int gender)
{
	if (gender != 1 && gender != 2)
		return nullptr;

	QVector<dorm*> candidates;//收集所有可用宿舍的指针(就地使用, 收集期间不发生增删, 指针有效)
	for (auto b_it = dorms.begin(); b_it != dorms.end(); ++b_it)
	{
		const building* b = buildingmanager::instance().get(b_it.key());
		if (b == nullptr)
			continue;
		if (!building_accepts_gender(b->get_for_gender(), gender))
			continue;

		for (auto d_it = b_it.value().begin(); d_it != b_it.value().end(); ++d_it)
		{
			const dorm& d = d_it.value();
			if (dorm_accepts_gender(d.get_for_gender(), gender) && !d.is_full())//房间性别接纳且未满
				candidates.append(&d_it.value());
		}
	}

	if (candidates.isEmpty())//无可用宿舍
		return nullptr;
	//bounded(n) 返回 [0, n) 的等概率随机数, 从候选中抽取一间
	return candidates[QRandomGenerator::global()->bounded(candidates.size())];
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

	//调用宿舍对象的clear_students()方法，自动清空宿舍内学生（同步清理 student 本体位置字段）
	dorms[building_id][dorm_id].clear_students();

	//从QMap<int, dorm>中移除宿舍对象，同步清理宿舍对象的内存空间
	dorms[building_id].remove(dorm_id);//移除宿舍对象
	if (dorms[building_id].isEmpty())//如果楼内没有宿舍了
		dorms.remove(building_id);//移除楼对象
	return true;
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
