#ifndef BUILDING_H
#define BUILDING_H

class building
{
public:
	building();

	//获取信息
	int get_id() const;//获取楼号
	int get_max_floor() const;//获取最大楼层数
	int get_for_gender() const;//获取宿舍楼适用性别
	bool accepts_gender(int gender) const;//判断本楼适用性别是否接纳学生性别: 混宿(3)接纳任意, 否则要求相等

	//操作信息
	bool set_id(int building_id);//设置宿舍楼号

private:
	//set_for_gender/set_max_floor 守护楼内 dorm 的跨聚合不变量:
	//改楼性别可能让既有宿舍性别锁定失配, 改最大楼层可能让既有 dorm_id 派生楼层越界。
	//building 自身无法检查 dormmanager，因此收 private；由 school 完成楼内一致性校验后调用 buildingmanager 写入。
	bool set_max_floor(int floor);//设置最大楼层数
	bool set_for_gender(int gender);//设置宿舍楼适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
	friend class buildingmanager;

	int id;//宿舍楼号
	int max_floor;//最大楼层数
	int for_gender;//宿舍适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
};

#endif // BUILDING_H
