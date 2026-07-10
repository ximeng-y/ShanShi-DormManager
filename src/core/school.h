#ifndef SCHOOL_H
#define SCHOOL_H

class student;
class dorm;
class building;

//学校顶层协调类。负责组合 studentmanager、dormmanager、buildingmanager
//完成涉及两个及以上同级 manager 的跨聚合业务，并向 UI 提供统一入口。
class school
{
public:
	static school& instance();//创建单例入口
	school(const school&) = delete;//禁止拷贝构造，维护单例唯一性
	school& operator=(const school&) = delete;//禁止拷贝赋值，维护单例唯一性

	//基础只读查询入口。返回指针均指向对应 manager 容器内部本体，后续删除对应对象后会失效；
	//student 指针还可能在 studentmanager 后续 add/remove 触发 QHash rehash 后失效。
	//请就地使用，不要长期持有；需长期引用请保存各对象 id，用时重新查询。
	const student* get_student(int student_id) const;//按学号获取学生本体，不存在返回 nullptr
	const dorm* get_dorm(int building_id, int dorm_id) const;//按楼号、宿舍号获取宿舍本体，不存在返回 nullptr
	const building* get_building(int building_id) const;//按楼号获取宿舍楼本体，不存在返回 nullptr
	int get_student_count() const;//获取全校学生总数
	int get_dorm_count() const;//获取全校宿舍总数
	int get_building_count() const;//获取全校宿舍楼总数

	//住宿协调
	//返回值: >0=成功(床位号)  -1=参数非法  -2=床位占用  -3=学生已有宿舍  -4=宿舍已满  -5=宿舍未配置  -6=学生不存在或性别未设置  -7=性别冲突  -8=宿舍不存在
	int assign_student_to_dorm(int building_id, int dorm_id, int student_id);//入住指定宿舍并自动分配最小空床位
	int assign_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id);//入住指定宿舍的指定床位
	//返回值: >0=成功(释放的床位号)  0=学生未入住  -1=参数非法  -6=学生不存在  -8=学生记录指向的宿舍不存在或床位记录不一致
	int remove_student_from_dorm(int student_id);//退宿但保留学籍

	//学籍协调
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -8=住宿记录不一致导致退宿失败
	int remove_student(int student_id);//退学籍，已入住时先退宿再删除学生本体
	//返回值: 1=成功  0=学生不存在  -1=参数非法  -7=所在楼/宿舍不接纳新性别或多人宿舍性别冲突  -8=住宿记录不一致
	int correct_student_gender(int student_id, int gender);//性别纠错，不自动迁宿

private:
	school() = default;//构造函数，单例模式禁止外部实例化
};

#endif // SCHOOL_H
