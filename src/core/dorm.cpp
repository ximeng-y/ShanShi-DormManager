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

	for (const student& s : students)//遍历查找该床位学生(动态列表模型下students不按床位号索引)
	{
		if (s.get_bed_id() == bed_id)
			return s.get_name();
	}
	return "error";//该床位未入住
}
int dorm::get_student_id(int bed_id) const//获取宿舍内指定床位学生学号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;

	for (const student& s : students)//在数组内遍历
	{
		if (s.get_bed_id() == bed_id)//尝试找到bed id对应传入bed id的学生，返回
			return s.get_id();
	}
	return -1;//该床位未入住
}
int dorm::get_student_class_num(int bed_id) const//获取宿舍内指定床位学生班级号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;

	for (const student& s : students)
	{
		if (s.get_bed_id() == bed_id)
			return s.get_class_num();
	}
	return -1;//该床位未入住
}

QVector<QString> dorm::get_student_name_list() const//获取宿舍内所有学生的名字表
{
	QVector<QString> name_list;
	for (const student& s : students)
		name_list.append(s.get_name());
	return name_list;
}
QVector<int> dorm::get_student_id_list() const//获取宿舍内所有学生的学号表
{
	QVector<int> id_list;
	for (const student& s : students)
		id_list.append(s.get_id());
	return id_list;
}
QVector<int> dorm::get_student_class_num_list() const//获取宿舍内所有学生的班级号表(不去重)
{
	QVector<int> class_list;
	for (const student& s : students)
		class_list.append(s.get_class_num());
	return class_list;
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

//判断学生是否存在
int dorm::is_student_exist(int bed_id) const//按床位号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;//bed_id非法
	for (const student& s : students)
	{
		if (s.get_bed_id() == bed_id)
			return 1;//存在学生
	}
	return 0;//床位为空
}
bool dorm::is_student_exist(const student& s) const//查找的是学生对象，但是按学号匹配（快速且唯一）
{
	for (const student& existing : students)
	{
		if (existing.get_id() == s.get_id())
			return true;
	}
	return false;
}

//添加学生: 自动同步学生的 bed_id / dorm_id / building_id
int dorm::add_student(student& s)//自动分配最小空床位
{
	if (is_student_exist(s))
		return -3;//学生已在本宿舍
	if (is_full())
		return -4;//宿舍已满

	int assigned = 1;
	while (assigned <= max_num && is_student_exist(assigned) == 1)
		++assigned;

	s.set_bed_id(assigned);
	s.set_dorm_id(this->id);
	s.set_building_id(this->building_id);
	s.set_floor(this->floor);
	students.append(s);
	return assigned;
}
int dorm::add_student(student& s, int bed_id)//指定床位
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;//bed_id非法
	if (is_student_exist(s))
		return -3;//学生已在本宿舍
	if (is_student_exist(bed_id) == 1)
		return -2;//床位已被占用

	s.set_bed_id(bed_id);
	s.set_dorm_id(this->id);
	s.set_building_id(this->building_id);
	s.set_floor(this->floor);
	students.append(s);
	return bed_id;
}

//移除学生
int dorm::remove_student(int bed_id)//按床位号
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return -1;//bed_id非法
	for (int i = 0; i < students.size(); ++i)
	{
		if (students[i].get_bed_id() == bed_id)
		{
			students.removeAt(i);
			return 1;//移除成功
		}
	}
	return 0;//该床位为空
}
bool dorm::remove_student(student& s)//按学号匹配, 移除后自动清零原对象位置信息
{
	for (int i = 0; i < students.size(); ++i)
	{
		if (students[i].get_id() == s.get_id())
		{
			students.removeAt(i);
			s.clear_dorm_info();//自动清零：移除后清除原对象的位置四字段
			return true;
		}
	}
	return false;
}

//调换/移动床位
int dorm::swap_student(int from, int to)
{
	if (!check::is_valid_bed_id(from, max_num) || !check::is_valid_bed_id(to, max_num))
		return -1;//bed_id非法
	if (from == to)
		return -2;//同一床位, 无需操作

	student* student_from = nullptr;
	student* student_to = nullptr;
	for (student& s : students)
	{
		if (s.get_bed_id() == from)
			student_from = &s;
		if (s.get_bed_id() == to)
			student_to = &s;
	}

	if (student_from == nullptr)
		return 0;//from床位为空

	if (student_to == nullptr)
		student_from->set_bed_id(to);//to为空, 移动
	else
	{
		student_from->set_bed_id(to);//to有人, 互换
		student_to->set_bed_id(from);
	}

	return 1;
}

const student* dorm::get_student(int bed_id) const//获取宿舍内指定床位学生对象
{
	if (!check::is_valid_bed_id(bed_id, max_num))
		return nullptr;
	for (const student& s : students)
	{
		if (s.get_bed_id() == bed_id)
			return &s;
	}
	return nullptr;//该床位为空
}

const student* dorm::get_student_by_id(int student_id) const//按学号查找学生对象
{
	for (const student& s : students)
	{
		if (s.get_id() == student_id)
			return &s;
	}
	return nullptr;
}

bool dorm::is_full() const//判断宿舍是否已满
{
	return students.size() >= max_num;
}

void dorm::clear_students()//清空所有学生
{
	students.clear();
}
