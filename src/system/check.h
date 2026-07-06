#ifndef CHECK_H
#define CHECK_H

#include <QString>

class check
{
public:
	static bool is_valid_student_id(int id);//检查学号是否合法(10000000~99999999)
	static bool is_valid_class_num(int class_num);//检查班级号是否合法(1~99)
	static bool is_valid_grade(int grade);//检查年级是否合法(2000~2999)
	static bool is_valid_dorm_id(int dorm_id);//检查宿舍号是否合法(1001~9999)
	static bool is_valid_building_id(int building_id);//检查宿舍楼号是否合法(1~99)
	static bool is_valid_bed_id(int bed_id, int max_num);//检查自然数床位号是否合法(1~max_num)
	static bool is_valid_floor(int floor, int max_floor);//检查楼层是否合法(1~max_floor)
	static bool is_valid_student_name(const QString& name);//检查学生姓名是否合法(1~20个字符,禁止error)
};


#endif // CHECK_H
