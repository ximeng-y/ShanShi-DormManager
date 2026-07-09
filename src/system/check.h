#ifndef CHECK_H
#define CHECK_H

#include <QString>

class check
{
public:
	static bool is_valid_student_id(int id);//检查学号是否合法(10000000~99999999)
	static bool is_valid_class_num(int class_num);//检查班级号是否合法(1~99)
	static bool is_valid_grade(int grade);//检查年级是否合法(2000~2999)
	static bool is_valid_dorm_id(int dorm_id);//检查宿舍号是否合法(101~9999 且末两位非00, 即3位1~9楼/4位10~99楼, 房间号01~99)
	static bool is_valid_dorm_floor(int dorm_id, int max_floor);//检查宿舍号派生楼层是否在楼最大楼层内(dorm_id/100 ∈ [1,max_floor])
	static bool is_valid_building_id(int building_id);//检查宿舍楼号是否合法(1~99)
	static bool is_valid_bed_id(int bed_id, int max_num);//检查自然数床位号是否合法(1~max_num)
	static bool is_valid_floor(int floor, int max_floor);//检查楼层是否合法(1~max_floor)
	static bool is_valid_student_name(const QString& name);//检查学生姓名是否合法(1~20个字符,禁止error)
	static bool is_valid_max_floor(int max_floor);//检查最大楼层数是否合法(1~99)
	static bool is_valid_gender(int gender);//检查性别是否合法(0=未设置, 1=男, 2=女)
	static bool is_valid_building_gender(int gender);//检查宿舍楼适用性别是否合法(1=男, 2=女, 3=男女混宿)
};


#endif // CHECK_H
