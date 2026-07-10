#ifndef STUDENTMANAGER_H
#define STUDENTMANAGER_H

#include <QHash>
#include <QVector>
#include "student.h"

//全校学生本体的唯一归属。以学号(id)为 key 存储 student 本体, 平均 O(1) 按学号查找。
//dorm/building 等空间结构一律只存学号, 通过本类换取本体, 保证单一数据源、无副本一致性问题。
class studentmanager
{
public:
	static studentmanager& instance();//创建单例入口
	studentmanager(const studentmanager&) = delete;//禁止拷贝构造，维护单例唯一性
	studentmanager& operator=(const studentmanager&) = delete;//禁止拷贝赋值，维护单例唯一性

	//操作学生
	//添加学生: 校验字段合法(name/class_num/grade/id)且学号唯一
	//返回值: true=成功  false=字段非法或学号已存在
	bool add(const student& s);
	bool remove(int student_id);//移除学生, true=移除成功, false=学号不存在或该生仍住宿舍(须先退宿, 防止学号残留在dorm.beds成幽灵)
	int clear_dorm_info(int student_id);//使指定学号的学生离宿：将bed_id、dorm_id、building_id、floor重置为0，供移除操作调用
	int assign_dorm_info(int student_id, int bed_id, int dorm_id, int building_id, int floor);//使指定学号的学生入住：经friend后门一次性写入位置四字段（绕过setter校验），供dorm::add_student成功分支调用，与clear_dorm_info对称
	int set_student_gender(int student_id, int gender);//修改学生性别, 1=成功/0=学号不存在/-1=gender非法
	int set_student_name(int student_id, const QString& name);//修改学生姓名, 1=成功/0=学号不存在/-1=姓名非法
	int set_student_class_num(int student_id, int class_num);//修改学生班级, 1=成功/0=学号不存在/-1=班级非法
	int set_student_grade(int student_id, int grade);//修改学生年级, 1=成功/0=学号不存在/-1=年级非法

	//按学号取本体(只读)。不存在返回 nullptr。
	//注意: 返回指针指向 QHash 内部, 在后续 add/remove 触发 rehash 后可能失效。
	//请就地使用, 不要长期持有; 需长期引用请存学号, 用时再 get。
	const student* get(int student_id) const;
	int count() const;//获取当前学生总数

	//逻辑判断
	int is_student_have_dorm(int student_id);//检查指定学号的学生是否已入住任意宿舍
	bool is_exists(int student_id) const;//判断指定学号的学生是否存在
	
	//高级查询: 手握全量, 一次遍历完成
	QVector<int> ids_of_class(int class_num) const;//列出指定班级号的所有学生学号
	QVector<int> all_ids() const;//列出全校所有学生学号(供全校随机分配等全量遍历使用)

private:
	studentmanager() = default;//构造函数, 单例模式禁止外部实例化
	QHash<int, student> students;
};

#endif // STUDENTMANAGER_H
