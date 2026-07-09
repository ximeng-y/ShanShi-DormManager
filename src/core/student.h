#ifndef STUDENT_H
#define STUDENT_H

#include <QString>
class studentmanager;//前向声明，降低编译时间，真实实现在studentmanager.h/cpp

class student//学生类，最基础的学生对象
{
public:
	student();//构造函数

	//获取信息
	QString get_name() const;//获取学生姓名
	int get_class_num() const;//获取学生班级
	int get_id() const;//获取学生学号
	int get_dorm_id() const;//获取所在宿舍号
	int get_bed_id() const;//获取所在床位号(自然数，从1开始)
	int get_grade() const;//获取学生年级
	int get_building_id() const;//获取所在宿舍楼号
	int get_floor() const;//获取所在楼层
	int get_gender() const;//获取性别(0=未设置, 1=男, 2=女)

	//设置信息
	bool set_name(const QString& name);//设置学生姓名
	bool set_class_num(int class_num);//设置学生班级
	bool set_id(int id);//设置学生学号
	bool set_grade(int grade);//设置学生年级
	bool set_gender(int gender);//设置性别(0=未设置, 1=男, 2=女)

private:
	//位置四字段(dorm_id/bed_id/building_id/floor)的 setter 收为 private:
	//床位占用的权威是 dorm.beds, 本体位置字段只应经 friend 后门 assign_dorm_info 同步写入。
	//公开这些 setter 会让本体"自称住某处"却无对应 beds 记录, 使 is_student_have_dorm 撒谎、remove 防护误判。
	bool set_dorm_id(int dorm_id);//设置所在宿舍号
	bool set_bed_id(int bed_id);//设置所在床位号(自然数，从1开始)
	bool set_building_id(int building_id);//设置所在宿舍楼号
	//set_floor 已删除: floor 不再独立校验写入, 改由 assign_dorm_info 后门接收 dorm 派生的 floor(id/100) 快照（与floor等价）

	//后门同步位置四字段, 绕过 setter 校验直接赋值。为未来的住宿协调层预留:
	//协调层在校验合法后调用它一次性写入 bed_id/dorm_id/building_id/floor。
	//本轮住宿协调未实现, 暂无调用者; 届时由协调类通过 friend 获得访问权(现已摘除 friend class dorm)。
	void assign_dorm_info(int bed_id, int dorm_id, int building_id, int floor);
	friend class studentmanager;

	QString name;
	int class_num;//班级号(1~99)
	int id;//前2位年级(的后两位) 3~4位班级 5~8位序列号
	int grade;//年级（4位，例如2026）
	int dorm_id;//所在宿舍号
	int bed_id;//所在床位号(自然数，从1开始)
	int building_id;//所在宿舍楼号
	int floor;//所在楼层
	int gender;//性别(0=未设置, 1=男, 2=女)
};

#endif // STUDENT_H
