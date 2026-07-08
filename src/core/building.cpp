#include <QVector>
#include "building.h"
#include "system/check.h"

building::building()
{
	id = 0;
	max_floor = 0;
	dorm_ids.clear();
}

//获取信息
int building::get_id()//获取楼号
{
	return id;
}

int building::get_max_floor()//获取最大楼层数
{
	return max_floor;
}

QVector<int> building::get_dorm_ids_list(int floor)//获取指定楼层号的宿舍号表
{
	return dorm_ids[floor - 1];//楼层号下标为自然数-1
}

QVector<QVector<int>> building::get_dorm_ids()//获取宿舍号二维表
{
	return dorm_ids;
}

int building::get_dorms_count()//获取整栋楼的宿舍数
{
	int count = 0;
	for (int i = 0; i < max_floor; i++) {
		count += dorm_ids[i].size();
	}
	return count;
}

int building::get_dorms_count(int floor)//获取指定楼层的宿舍数
{
	return dorm_ids[floor - 1].size();//楼层号下标为自然数-1
}

//操作信息
bool building::set_id(int building_id)//设置宿舍楼号
{
	if (!check::is_valid_building_id(building_id))
		return false;
	this->id = building_id;
	return true;
}
bool building::set_max_floor(int floor)//设置最大楼层数
{
	if (!check::is_valid_max_floor(floor))
		return false;
	this->max_floor = floor;
	return true;
}
int building::add_dorm(int floor);//向指定层新增一个宿舍（无宿舍号参数指定，默认填入最小可用顺位（如已有401、403、404则填入402））
int building::add_dorm(int floor, int dorm_id);//向指定层新增一个指定号宿舍
int building::remove_dorm(int floor);//减少指定层的一个宿舍（无宿舍号参数指定，默认删除最大宿舍号）
int building::remove_dorm(int floor, int dorm_id);//减少指定层的一个指定号宿舍
