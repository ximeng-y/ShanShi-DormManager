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
	int set_student_name(int student_id, const QString& name);//修改学生姓名, 1=成功/0=学号不存在/-1=姓名非法

	//按学号取本体(只读)。不存在返回 nullptr。
	//注意: 返回指针指向 QHash 内部, 在后续 add/remove 触发 rehash 后可能失效。
	//请就地使用, 不要长期持有; 需长期引用请存学号, 用时再 get。
	const student* get(int student_id) const;
	int count() const;//获取当前学生总数

	//逻辑判断
	int is_student_have_dorm(int student_id);//检查指定学号的学生是否已入住任意宿舍
	bool is_exists(int student_id) const;//判断指定学号的学生是否存在
	bool is_sequence_used(int grade, int sequence, int except_student_id = 0) const;//同年级是否已有学生使用指定序号
	int next_available_sequence(int grade, int except_student_id = 0) const;//最小可用序号，0=耗尽/-1=参数非法
	
	//高级查询: 手握全量, 一次遍历完成
	QVector<int> ids_of_class(int class_num) const;//列出指定班级号的所有学生学号
	QVector<int> all_ids() const;//列出全校所有学生学号(供全校随机分配等全量遍历使用)

private:
	studentmanager() = default;//构造函数, 单例模式禁止外部实例化
	bool remove(int student_id);//移除学生本体，跨聚合退宿由 school 前置完成
	int clear_dorm_info(int student_id);//清零住宿位置四字段，仅供 school 协调
	int assign_dorm_info(int student_id, int bed_id, int dorm_id, int building_id, int floor);//写入住宿位置四字段，仅供 school 协调
	int set_student_gender(int student_id, int gender);//修改学生性别，仅供 school 完成住宿一致性校验后调用
	int rekey_student(int old_student_id, int new_student_id, int new_grade, int new_class_num);//只替换学生主键和学籍身份字段
	friend class school;
	QHash<int, student> students;
};

#endif // STUDENTMANAGER_H
