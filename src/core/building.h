#ifndef BUILDING_H
#define BUILDING_H

#include <QVector>

class building
{
public:
	building();

	//获取信息
	int get_id();//获取楼号
	int get_max_floor();//获取最大楼层数
	QVector<int> get_dorm_ids_list(int floor);//获取指定楼层号的宿舍号表
	QVector<QVector<int>> get_dorm_ids();//获取宿舍号二维表
	int get_dorms_count();//获取整栋楼的宿舍数
	int get_dorms_count(int floor);//获取指定楼层的宿舍数
	int get_for_gender();//获取宿舍适用性别

	//操作信息
	bool set_id(int building_id);//设置宿舍楼号
	bool set_max_floor(int floor);//设置最大楼层数
	bool set_for_gender(int gender);//设置宿舍适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
	int add_dorm(int floor);//向指定层新增一个宿舍（无宿舍号参数指定，默认填入最小可用顺位（如已有401、403、404则填入402））
	int add_dorm(int floor, int dorm_id);//向指定层新增一个指定号宿舍
	int remove_dorm(int floor);//减少指定层的一个宿舍（无宿舍号参数指定，默认删除最大宿舍号）
	int remove_dorm(int floor, int dorm_id);//减少指定层的一个指定号宿舍

private:
	int id;//宿舍楼号
	int max_floor;//最大楼层数
	int for_gender;//宿舍适用性别，1为男，2为女，0为无性别（非法），3为男女混宿
	QVector<QVector<int>> dorm_ids;//每层的宿舍号列表(二维数组，第一维为楼层号下标(自然数-1)，第二维为该楼层的宿舍号列表)
};

#endif // BUILDING_H