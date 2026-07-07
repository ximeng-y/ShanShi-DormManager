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
	studentmanager();//构造函数

	//本体管理
	//添加学生: 校验字段合法(name/class_num/grade/id)且学号唯一。
	//返回值: true=成功  false=字段非法或学号已存在
	bool add(const student& s);
	bool remove(int student_id);//移除学生, true=移除成功, false=学号不存在
	bool is_exists(int student_id) const;//判断指定学号的学生是否存在
	//按学号取本体(可修改)。不存在返回 nullptr。
	//注意: 返回指针指向 QHash 内部, 在后续 add/remove 触发 rehash 后可能失效。
	//请就地使用, 不要长期持有; 需长期引用请存学号, 用时再 get。
	student* get(int student_id);
	const student* get(int student_id) const;//const 重载, 返回只读指针
	int count() const;//当前学生总数

	//高级查询: 手握全量, 一次遍历完成
	QVector<int> ids_of_class(int class_num) const;//列出指定班级号的所有学生学号

private:
	QHash<int, student> students;//使用哈希表存储学生学号 -> 学生本体
};

#endif // STUDENTMANAGER_H
