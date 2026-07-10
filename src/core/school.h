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

private:
	school() = default;//构造函数，单例模式禁止外部实例化
};

#endif // SCHOOL_H
