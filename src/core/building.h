#ifndef BUILDING_H
#define BUILDING_H

#include <QVector>

class building
{
public:
	building();

	//获取信息
	int get_id() const;//获取楼号
	int get_max_floor() const;//获取最大楼层数
	QVector<int> get_dorm_ids_list(int floor) const;//获取指定楼层号的宿舍号表
	QVector<QVector<int>> get_dorm_ids() const;//获取宿舍号二维表
	int get_dorms_count() const;//获取整栋楼的宿舍数
	int get_dorms_count(int floor) const;//获取指定楼层的宿舍数
	int get_for_gender() const;//获取宿舍楼适用性别
	bool accepts_gender(int gender) const;//判断本楼适用性别是否接纳学生性别: 混宿(3)接纳任意, 否则要求相等

	//操作信息
	bool set_id(int building_id);//设置宿舍楼号

private:
	//set_for_gender/set_max_floor 守护楼内 dorm 的跨聚合不变量:
	//改楼性别可能让既有宿舍性别锁定失配, 改最大楼层可能让既有 dorm_id 派生楼层越界。
	//building 自身无法检查 dormmanager, 因此收 private, 由 buildingmanager 包装或未来 school 协调。
	bool set_max_floor(int floor);//设置最大楼层数
	bool set_for_gender(int gender);//设置宿舍楼适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
	friend class buildingmanager;

	int id;//宿舍楼号
	int max_floor;//最大楼层数
	int for_gender;//宿舍适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
	QVector<QVector<int>> dorm_ids;//每层的宿舍号列表(二维数组，第一维为楼层号下标(自然数-1)，第二维为该楼层的宿舍号列表)
};

#endif // BUILDING_H
