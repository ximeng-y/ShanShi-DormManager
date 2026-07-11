#include "buildingmanager.h"
#include "system/check.h"

buildingmanager& buildingmanager::instance()
{
	static buildingmanager mgr;
	return mgr;
}

//添加宿舍楼: 校验字段合法且楼号唯一
bool buildingmanager::add_building(const building& b)
{
	//校验楼栋自身字段(与 student/dorm 的前置校验保持一致的合法性标准)
	if (!check::is_valid_building_id(b.get_id()) ||
		!check::is_valid_building_gender(b.get_for_gender()) ||
		!check::is_valid_max_floor(b.get_max_floor()))
		return false;//字段非法
	if (buildings.contains(b.get_id()))
		return false;//楼号已存在(唯一性约束)

	buildings.insert(b.get_id(), b);
	return true;
}

int buildingmanager::add_building(int building_id, int gender, int max_floor)//构造并添加宿舍楼
{
	if (!check::is_valid_building_id(building_id) ||
		!check::is_valid_building_gender(gender) ||
		!check::is_valid_max_floor(max_floor))
		return -1;//字段非法
	if (buildings.contains(building_id))
		return 0;//楼号已存在

	building b;
	b.set_id(building_id);
	b.set_for_gender(gender);
	b.set_max_floor(max_floor);
	buildings.insert(building_id, b);
	return 1;
}

//移除宿舍楼(仅从表中删除, 不级联清理楼内宿舍, 该协调留待上层 school)
bool buildingmanager::remove_building(int building_id)
{
	return buildings.remove(building_id) > 0;//remove 返回移除个数, >0 表示成功
}

//按楼号取本体(只读)
const building* buildingmanager::get(int building_id) const
{
	auto it = buildings.constFind(building_id);
	if (it == buildings.constEnd())
		return nullptr;
	return &it.value();
}

int buildingmanager::set_building_gender(int building_id, int gender)//修改楼适用性别
{
	if (!check::is_valid_building_gender(gender))
		return -1;//gender非法
	auto it = buildings.find(building_id);
	if (it == buildings.end())
		return 0;//楼不存在
	return it.value().set_for_gender(gender) ? 1 : -1;
}

int buildingmanager::set_building_max_floor(int building_id, int floor)//修改楼最大楼层
{
	if (!check::is_valid_max_floor(floor))
		return -1;//floor非法
	auto it = buildings.find(building_id);
	if (it == buildings.end())
		return 0;//楼不存在
	return it.value().set_max_floor(floor) ? 1 : -1;
}

//检查指定楼号是否存在
int buildingmanager::is_building_exist(int building_id)
{
	if (!check::is_valid_building_id(building_id))
		return -1;//参数非法
	return buildings.contains(building_id) ? 1 : 0;//存在返回1, 否则0
}

//获取当前宿舍楼总数
int buildingmanager::count() const
{
	return buildings.size();
}

QVector<int> buildingmanager::all_ids() const//按楼号升序列出全部宿舍楼
{
	QVector<int> ids;
	for (auto it = buildings.constBegin(); it != buildings.constEnd(); ++it)
		ids.append(it.key());
	return ids;
}
