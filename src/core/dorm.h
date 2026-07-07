#ifndef DORM_H
#define DORM_H

#include <QString>
#include <QVector>
#include "student.h"

class dorm
{
public:
	dorm();//构造函数

	//获取信息
	int get_id() const;//获取宿舍号
	int get_max_num() const;//获取最大人数
	int get_current_num() const;//获取实际人数
	int get_building_id() const;//获取所在宿舍楼号
	int get_floor() const;//获取所在楼层
	QString get_student_name(int bed_id) const;//获取宿舍内指定床位学生姓名
	int get_student_id(int bed_id) const;//获取宿舍内指定床位学生学号
	int get_student_class_num(int bed_id) const;//获取宿舍内指定床位学生班级号
	QVector<QString> get_student_name_list() const;//获取宿舍内所有学生的名字表
	QVector<int> get_student_id_list() const;//获取宿舍内所有学生的学号表
	QVector<int> get_student_class_num_list() const;//获取宿舍内所有学生的班级号表(不去重)
	const student* get_student(int bed_id) const;//获取宿舍内指定床位学生对象(返回nullptr表示bed_id非法或该床位为空)
	const student* get_student_by_id(int student_id) const;//按学号查找学生对象(返回nullptr表示未找到)

	//返回值: 1=存在学生  0=床位为空  -1=bed_id非法
	int is_student_exist(int bed_id) const;//判断宿舍内指定床位是否有人(按床位号)
	bool is_student_exist(const student& s) const;//判断宿舍内是否存在指定学生(按学号匹配)


	//设置信息
	bool set_id(int id);//设置宿舍号
	bool set_max_num(int max_num);//设置最大人数
	bool set_building_id(int building_id);//设置所在宿舍楼号
	bool set_floor(int floor);//设置所在楼层

	//管理学生: add_student 自动同步学生的 bed_id / dorm_id / building_id / floor
	//返回值: >0=成功(即分配的床位号)  -3=学生已在本宿舍  -4=宿舍已满
	int add_student(student& s);//添加学生(自动分配最小空床位)
	//返回值: >0=成功(即指定床位号)  -1=bed_id非法  -2=床位已被占用  -3=学生已在本宿舍
	int add_student(student& s, int bed_id);//添加学生(指定床位)

	//返回值: 1=移除成功  0=该床位为空  -1=bed_id非法
	int remove_student(int bed_id);//移除学生(按床位号)
	//移除后自动清零原对象的bed_id/dorm_id/building_id/floor（通过student::clear_dorm_info）
	bool remove_student(student& s);//移除学生(按学号匹配, 找到并移除返回true, 未找到返回false)

	//返回值: 1=成功(to空则移入, to有人则互换)  0=from床位为空  -1=bed_id非法  -2=from==to
	int swap_student(int from, int to);//调换/移动床位(from→to)

	bool is_full() const;//判断宿舍是否已满
	void clear_students();//清空所有学生（注意：已通过get_student/get_student_by_id获取的指针将失效）

private:
	int id;//宿舍号
	int max_num;//最大人数
	int building_id;//所在宿舍楼号(1~99)
	int floor;//所在楼层(1~max_floor)
	QVector<student> students;//宿舍内学生
};

#endif // DORM_H
