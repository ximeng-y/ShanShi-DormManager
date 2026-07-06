#include "dorm.h"
#include "student.h"
#include <QString>
#include <QVector>
#include "system/check.h"

dorm::dorm()//构造函数
{
	id = 0;
	max_num = 0;
	building_id = 0;
	floor = 0;
	students.clear();
}

//获取信息
int dorm::get_id() const//获取宿舍号
{
	return id;
}
int dorm::get_max_num() const//获取最大人数
{
	return max_num;
}
int dorm::get_current_num() const//获取实际人数
{
	return students.size();
}
int dorm::get_building_id() const//获取所在宿舍楼号
{
	return building_id;
}
int dorm::get_floor() const//获取所在楼层
{
	return floor;
}

QString dorm::get_student_name(int bed_id) const//获取宿舍内指定床位学生姓名(id使用自然数，从1开始)
{
	if (!check::is_valid_bed_id(bed_id, max_num))//检查床位号是否合法,不合法返回error
		return "error";
	int true_bed_id = bed_id - 1;//将自然数床位号转换为索引

	return students[true_bed_id].get_name();
}
int dorm::get_student_id(int bed_id) const//获取宿舍内指定床位学生学号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;
	int true_bed_id = bed_id - 1;

	return students[true_bed_id].get_id();
}
int dorm::get_student_class_num(int bed_id) const//获取宿舍内指定床位学生班级号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;
	int true_bed_id = bed_id - 1;

	return students[true_bed_id].get_class_num();
}

//设置信息
bool dorm::set_id(int id)//设置宿舍号
{
	if (!check::is_valid_dorm_id(id))
		return false;
	this->id = id;
	return true;
}
bool dorm::set_max_num(int max_num)//设置最大人数
{
	if (max_num < 1)
		return false;
	this->max_num = max_num;
	return true;
}
bool dorm::set_building_id(int building_id)//设置所在宿舍楼号
{
	if (!check::is_valid_building_id(building_id))
		return false;
	this->building_id = building_id;
	return true;
}
bool dorm::set_floor(int floor)//设置所在楼层
{
	int max_floor = 99;//此处为暂时的设置，后续会设计不同宿舍楼的最大楼层数不同，初步打算用顶层类定义vector管理，在此处定义常量获取顶层类vector中的对应宿舍楼最大楼层数
	if (!check::is_valid_floor(floor, max_floor))
		return false;
	this->floor = floor;
	return true;
}
