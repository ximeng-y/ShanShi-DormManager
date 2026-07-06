#ifndef STUDENT_H
#define STUDENT_H

#include <QString>

class student
{
public:
	student();//构造函数

	//获取信息
	QString get_name() const;//获取学生姓名
	int get_class_num() const;//获取学生班级
	int get_id() const;//获取学生学号

	//设置信息
	void set_name(const QString& name);//设置学生姓名
	void set_class_num(int class_num);//设置学生班级
	void set_id(int id);//设置学生学号

private:
	QString name;
	int class_num;
	int id;//前2位年级 3~4位班级 5~8位序列号
};

#endif // STUDENT_H
