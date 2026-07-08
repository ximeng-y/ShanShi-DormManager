#include "dormmanager.h"
#include "system/check.h"

dormmanager& dormmanager::instance()
{
	static dormmanager mgr;
	return mgr;
}

//按楼号、宿舍号取本体（可修改）
dorm* dormmanager::get(int building_id, int dorm_id)
{
	auto building_it = dorms.find(building_id);
	if (building_it == dorms.end())
		return nullptr;
	auto dorm_it = building_it->find(dorm_id);
	if (dorm_it == building_it->end())
		return nullptr;
	return &dorm_it.value();
}

//按楼号、宿舍号取本体（只读）
const dorm* dormmanager::get(int building_id, int dorm_id) const
{
	auto building_it = dorms.constFind(building_id);
	if (building_it == dorms.constEnd())
		return nullptr;
	auto dorm_it = building_it->constFind(dorm_id);
	if (dorm_it == building_it->constEnd())
		return nullptr;
	return &dorm_it.value();
}

//获取当前宿舍总数
int dormmanager::count() const
{
	int total = 0;
	for (auto it = dorms.constBegin(); it != dorms.constEnd(); ++it)
		total += it.value().size();
	return total;
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

//添加宿舍（以参数 building_id 为准，同步 dorm 内部 building_id）
bool dormmanager::add_dorm(int building_id, const dorm& dorm_to_add)
{
	int dorm_id = dorm_to_add.get_id();

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

	//局部拷贝并同步 building_id（参数为准，自动同步）
	dorm dorm_copy = dorm_to_add;
	dorm_copy.set_building_id(building_id);

	dorms[building_id].insert(dorm_id, dorm_copy);
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

	//自动清空宿舍内学生（同步清理 student 本体位置字段）
	dorms[building_id][dorm_id].clear_students();

	//移除
	dorms[building_id].remove(dorm_id);
	if (dorms[building_id].isEmpty())
		dorms.remove(building_id);
	return true;
}
