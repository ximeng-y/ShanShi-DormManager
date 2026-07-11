#ifndef DORMMANAGER_H
#define DORMMANAGER_H

#include "dorm.h"
#include <QVector>
#include <QMap>
#include <QPair>

// 全校宿舍本体的唯一归属。两层 QMap 以 (宿舍楼号, 宿舍号) 复合键定位 dorm 本体, 按楼号、宿舍号升序有序存储。
//宿舍目录统一由本类维护，其它层保存或传递复合键，用时通过本类换取本体。
class dormmanager
{
public:
	static dormmanager& instance();//创建单例入口
    dormmanager(const dormmanager&) = delete;//禁止拷贝构造，维护单例唯一性
	dormmanager& operator=(const dormmanager&) = delete;//禁止拷贝赋值，维护单例唯一性

    //操作宿舍
	int set_dorm_max_num(int building_id, int dorm_id, int max_num);//修改宿舍最大人数, 1=成功/0=宿舍不存在/-1=参数非法/-2=缩容丢人

	//===== 空床位总数统计(注意与已有 get_empty_count()=空宿舍数区分) =====
	//无参: 全校空床位总数。带 building_id: 限定某楼，只统计 dormmanager 当前持有的宿舍。
	//返回值: >=0=床位数  -1=building_id 格式非法
	int get_empty_bed_count() const;
	int get_empty_bed_count_of_building(int building_id) const;

    //按楼号、宿舍号取本体(只读)。不存在返回 nullptr。
	//注意: 返回指针指向 QMap 内部。QMap 为红黑树, 插入不会使已有项引用失效; 但删除被指向的项后指针失效。
	//请就地使用, 不要长期持有; 需长期引用请存宿舍楼号, 用时再 get。
    const dorm* get(int building_id, int dorm_id) const;
	QVector<QPair<int, int>> all_dorm_keys() const;//按楼号、宿舍号升序列出全部宿舍复合键

    //逻辑判断
    int is_dorm_exist(int building_id, int dorm_id);//检查宿舍是否存在，1=存在/0=楼或宿舍不存在/-1=参数格式非法

    //高级信息查询
    int count() const;//获取当前宿舍总数。获取指定楼号的宿舍数等功能可调用get()通过building类的方法实现
    int get_empty_count() const;//获取当前空宿舍总数
    int get_occupied_count() const;//获取当前已占用宿舍总数

private:
	dormmanager() = default;//构造函数, 单例模式禁止外部实例化
	//仅供 school 完成两端同步闭环的床位写接口，不得由普通调用者直接使用。
	bool add_dorm(const dorm& dorm_to_add);//添加宿舍本体，只校验宿舍自身字段与同楼唯一性
	bool remove_dorm(int building_id, int dorm_id);//删除指定宿舍本体，不同步学生位置字段
	int set_dorm_gender(int building_id, int dorm_id, int gender);//设置房间性别锁，只校验参数与宿舍存在性
	int add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender);//向指定宿舍写入学生学号, 不查询或同步学生本体, -8=宿舍不存在
	int add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender, int bed_id);//向指定床位写入学生学号, 不查询或同步学生本体, -8=宿舍不存在
	int remove_student_from_dorm(int building_id, int dorm_id, int student_id);//从指定宿舍移除学生学号, 不同步学生本体, -8=宿舍不存在
	//在同一宿舍内移动或交换床位，不同步学生本体。
	//返回值: 1=成功  0=源床位为空  -1=楼号/宿舍号/床位号非法  -2=源与目标为同一床位  -8=宿舍不存在
	int move_student_bed(int building_id, int dorm_id, int from_bed_id, int to_bed_id);
	int clear_dorm_students(int building_id, int dorm_id, bool reset_gender);//清空指定宿舍住客，可选放开性别锁，返回清空人数/-8不存在
	friend class school;
	//内部定位: 按楼号、宿舍号返回可写本体指针(供内部搬迁改写用), 不存在返回 nullptr。
	//对外仍只暴露 const get(); 本 helper 不校验 id 格式(调用方负责)。
	dorm* find_dorm(int building_id, int dorm_id);
	QMap<int, QMap<int, dorm>> dorms;//宿舍本体有序表, 外层key为宿舍楼号, value为该楼的宿舍本体有序表, 内层key为宿舍号, value为宿舍本体。QMap按key升序, 遍历天然按楼号、宿舍号顺序
};

#endif // DORMMANAGER_H
