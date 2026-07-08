#include "studentmanager.h"
#include "system/check.h"
#include "student.h"

studentmanager& studentmanager::instance()
{
	static studentmanager mgr;
	return mgr;
}

//操作学生
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
	return students.remove(student_id) > 0;//remove 返回true表示移除成功, false=学号不存在
}

bool studentmanager::is_exists(int student_id) const//判断学生是否存在
{
	return students.contains(student_id);//直接返回是否存在
}

student* studentmanager::get(int student_id)//按学号取本体(可修改)
{
	auto it = students.find(student_id);//此处使用find在哈希表中快速定位
	if (it == students.end())//.end是哈希表的末尾迭代器, 表示未找到
		return nullptr;
	return &it.value();//返回指向学生本体的指针
}

const student* studentmanager::get(int student_id) const//按学号取本体（只读）
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

int studentmanager::clear_dorm_info(int student_id)//使指定学号的学生离宿：将bed_id、dorm_id、building_id、floor重置为0，供移除操作调用，约定返回值-1为学号不存在，1为成功
{
	if (!check::is_valid_student_id(student_id) || !is_exists(student_id))//如果学号非法/学生不存在
		return -1;//学号不存在

	student* s = get(student_id);

	s->assign_dorm_info(0, 0, 0, 0);
	return 1;
}

int studentmanager::assign_dorm_info(int student_id, int bed_id, int dorm_id, int building_id, int floor)//使指定学号的学生入住：经friend后门一次性写入位置四字段（绕过setter校验），供dorm::add_student成功分支调用，与clear_dorm_info对称，约定返回值-1为学号不存在，1为成功
{
	if (!check::is_valid_student_id(student_id) || !is_exists(student_id))//如果学号非法/学生不存在
		return -1;//学号不存在
	student* s = get(student_id);
	s->assign_dorm_info(bed_id, dorm_id, building_id, floor);
	return 1;
}

//逻辑判断
int studentmanager::is_student_have_dorm(int student_id)//检查指定学号的学生是否已入住任意宿舍
{
	const student* s = get(student_id);
	if (!check::is_valid_student_id(student_id) || !is_exists(student_id))//如果学号非法/学生不存在
		return -1;//学号不存在
	if (s->get_dorm_id() == 0 || s->get_bed_id() == 0)
		return 0;//未分配宿舍或床位
	return 1;//已分配宿舍、床位
}

//高级查询
QVector<int> studentmanager::ids_of_class(int class_num) const//列出指定班级的所有学生学号
{
	QVector<int> id_list;
	for (auto it = students.constBegin(); it != students.constEnd(); ++it)//constBegin/End为只读调用 其中Begin代表哈希表的首个元素
	{
		if (it.value().get_class_num() == class_num)//如果学生的班级号匹配
			id_list.append(it.key());
	}
	return id_list;
}
