#include "studentmanager.h"
#include "system/check.h"
#include "student.h"
#include <QBitArray>

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
		!check::is_valid_gender(s.get_gender()) ||
		!check::is_student_id_consistent(s.get_id(), s.get_grade(), s.get_class_num()))
		return false;//字段非法
	if (s.get_bed_id() != 0 || s.get_dorm_id() != 0 || s.get_building_id() != 0 || s.get_floor() != 0)
		return false;//禁止携带住宿位置快照入库，所有入住必须经过school
	if (students.contains(s.get_id()))
		return false;//学号已存在(唯一性约束)
	if (is_sequence_used(s.get_grade(), check::student_id_sequence(s.get_id())))
		return false;//同一年级内序号必须唯一，忽略班级

	students.insert(s.get_id(), s);
	return true;
}

bool studentmanager::remove(int student_id)//移除学生
{
	if (is_student_have_dorm(student_id) == 1)//防护: 学生仍住宿舍时拒绝删除, 否则学号会留在 dorm.beds 里成幽灵
		return false;//还住着宿舍，须通过school先退宿再删人
	return students.remove(student_id) > 0;//remove 返回移除个数, >0=移除成功, 0=学号不存在
}

bool studentmanager::is_exists(int student_id) const//判断学生是否存在
{
	return students.contains(student_id);//直接返回是否存在
}

bool studentmanager::is_sequence_used(int grade, int sequence, int except_student_id) const//同年级是否已有学生使用指定序号
{
	if (!check::is_valid_grade(grade) || !check::is_valid_student_sequence(sequence))
		return false;
	for (auto it = students.constBegin(); it != students.constEnd(); ++it)
	{
		if (it.key() == except_student_id || it.value().get_grade() != grade)
			continue;
		if (check::student_id_sequence(it.value().get_id()) == sequence)
			return true;
	}
	return false;
}

int studentmanager::next_available_sequence(int grade, int except_student_id) const//查找同年级最小未占用序号
{
	if (!check::is_valid_grade(grade))
		return -1;
	QBitArray used(10000, false);
	for (auto it = students.constBegin(); it != students.constEnd(); ++it)
	{
		if (it.key() == except_student_id || it.value().get_grade() != grade)
			continue;
		const int sequence = check::student_id_sequence(it.value().get_id());
		if (check::is_valid_student_sequence(sequence))
			used.setBit(sequence);
	}
	for (int sequence = 1; sequence <= 9999; ++sequence)
	{
		if (!used.testBit(sequence))
			return sequence;
	}
	return 0;
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

	students[student_id].assign_dorm_info(0, 0, 0, 0);
	return 1;
}

int studentmanager::assign_dorm_info(int student_id, int bed_id, int dorm_id, int building_id, int floor)//使指定学号的学生入住：经friend后门一次性写入位置四字段（绕过setter校验），供school完成住宿同步，与clear_dorm_info对称，约定返回值-1为学号不存在，1为成功
{
	if (!check::is_valid_student_id(student_id) || !is_exists(student_id))//如果学号非法/学生不存在
		return -1;//学号不存在
	students[student_id].assign_dorm_info(bed_id, dorm_id, building_id, floor);
	return 1;
}

int studentmanager::set_student_gender(int student_id, int gender)//修改学生性别
{
	if (!check::is_valid_gender(gender))
		return -1;//gender非法
	auto it = students.find(student_id);
	if (it == students.end())
		return 0;//学号不存在
	return it.value().set_gender(gender) ? 1 : -1;
}

int studentmanager::rekey_student(int old_student_id, int new_student_id, int new_grade, int new_class_num)//替换学生主键和学籍身份字段
{
	if (!check::is_valid_student_id(old_student_id)
		|| !check::is_student_id_consistent(new_student_id, new_grade, new_class_num))
		return -1;
	auto old_it = students.constFind(old_student_id);
	if (old_it == students.constEnd())
		return 0;
	if (old_student_id != new_student_id && students.contains(new_student_id))
		return -2;
	if (is_sequence_used(new_grade, check::student_id_sequence(new_student_id), old_student_id))
		return -2;
	student updated = old_it.value();
	updated.assign_academic_identity(new_student_id, new_grade, new_class_num);
	students.remove(old_student_id);
	students.insert(new_student_id, updated);
	return 1;
}

int studentmanager::set_student_name(int student_id, const QString& name)//修改学生姓名
{
	if (!check::is_valid_student_name(name))
		return -1;//姓名非法
	auto it = students.find(student_id);
	if (it == students.end())
		return 0;//学号不存在
	return it.value().set_name(name) ? 1 : -1;
}

int studentmanager::set_student_class_num(int student_id, int class_num)//修改学生班级
{
	if (!check::is_valid_class_num(class_num))
		return -1;//班级非法
	auto it = students.find(student_id);
	if (it == students.end())
		return 0;//学号不存在
	return it.value().set_class_num(class_num) ? 1 : -1;
}

int studentmanager::set_student_grade(int student_id, int grade)//修改学生年级
{
	if (!check::is_valid_grade(grade))
		return -1;//年级非法
	auto it = students.find(student_id);
	if (it == students.end())
		return 0;//学号不存在
	return it.value().set_grade(grade) ? 1 : -1;
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

QVector<int> studentmanager::all_ids() const//列出全校所有学生学号
{
	QVector<int> id_list;
	for (auto it = students.constBegin(); it != students.constEnd(); ++it)
		id_list.append(it.key());
	return id_list;
}
