#include "studentmanager.h"
#include "system/check.h"

studentmanager::studentmanager()//构造函数
{
	students.clear();
}

//本体管理
bool studentmanager::add(const student& s)//添加学生: 校验字段合法且学号唯一
{
	//校验学生自身字段(与 dorm 旧 add_student 的前置校验保持一致的合法性标准)
	if (!check::is_valid_student_name(s.get_name()) ||
		!check::is_valid_class_num(s.get_class_num()) ||
		!check::is_valid_grade(s.get_grade()) ||
		!check::is_valid_student_id(s.get_id()))
		return false;//字段非法
	if (students.contains(s.get_id()))
		return false;//学号已存在(唯一性约束)

	students.insert(s.get_id(), s);
	return true;
}
bool studentmanager::remove(int student_id)//移除学生
{
	return students.remove(student_id) > 0;//remove 返回被删除的元素个数
}
bool studentmanager::is_exists(int student_id) const//判断学生是否存在
{
	return students.contains(student_id);
}
student* studentmanager::get(int student_id)//按学号取本体(可修改)
{
	auto it = students.find(student_id);
	if (it == students.end())
		return nullptr;
	return &it.value();
}
const student* studentmanager::get(int student_id) const//const 重载
{
	auto it = students.constFind(student_id);
	if (it == students.constEnd())
		return nullptr;
	return &it.value();
}
int studentmanager::count() const//当前学生总数
{
	return students.size();
}

//高级查询
QVector<int> studentmanager::ids_of_class(int class_num) const//列出指定班级的所有学生学号
{
	QVector<int> id_list;
	for (auto it = students.constBegin(); it != students.constEnd(); ++it)
	{
		if (it.value().get_class_num() == class_num)
			id_list.append(it.key());
	}
	return id_list;
}
