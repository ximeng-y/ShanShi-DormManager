#include "building.h"
#include "system/check.h"

building::building()
{
	id = 0;
	max_floor = 0;
	for_gender = 0;
}

//获取信息
int building::get_id() const//获取楼号
{
	return id;
}

int building::get_max_floor() const//获取最大楼层数
{
	return max_floor;
}

//操作信息
bool building::set_id(int building_id)//设置宿舍楼号
{
	if (!check::is_valid_building_id(building_id))
		return false;
	this->id = building_id;
	return true;
}

int building::get_for_gender() const//获取宿舍适用性别
{
	return for_gender;
}

bool building::accepts_gender(int gender) const//判断本楼适用性别是否接纳学生性别: 混宿(3)接纳任意, 否则要求相等
{
	return for_gender == 3 || for_gender == gender;
}

bool building::set_for_gender(int gender)//设置宿舍适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
{
	if (!check::is_valid_building_gender(gender))
		return false;
	this->for_gender = gender;
	return true;
}

bool building::set_max_floor(int floor)//设置最大楼层数
{
	if (!check::is_valid_max_floor(floor))
		return false;
	this->max_floor = floor;
	return true;
}
