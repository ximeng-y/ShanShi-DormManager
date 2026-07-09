#ifndef DORMMANAGER_H
#define DORMMANAGER_H

#include "dorm.h"
#include <QVector>
#include <QMap>

// 全校宿舍本体的唯一归属。两层 QMap 以 (宿舍楼号, 宿舍号) 复合键定位 dorm 本体, 按楼号、宿舍号升序有序存储。
// building 等空间结构一律只存宿舍编号, 通过本类换取本体, 保证单一数据源、无副本一致性问题。
class dormmanager
{
public:
	static dormmanager& instance();//创建单例入口
    dormmanager(const dormmanager&) = delete;//禁止拷贝构造，维护单例唯一性
	dormmanager& operator=(const dormmanager&) = delete;//禁止拷贝赋值，维护单例唯一性

    //操作宿舍
    bool add_dorm(const dorm& dorm_to_add);//添加宿舍（building_id 取自 dorm 内部）
    bool remove_dorm(int building_id, int dorm_id);//删除处于指定楼号的宿舍（单个楼内的宿舍号是唯一的）

    //钦定房间性别锁定(经 friend 后门调 dorm::set_for_gender, 前置做楼-房一致性 G2 校验)。
    //gender=0 解锁时跳过楼级校验; gender=1/2 须所在 building 接纳该性别, 否则拒绝(防「死间」)。
    //返回值: 1=成功  0=宿舍不存在  -1=参数非法(building_id/dorm_id/gender 格式)  -2=楼未注册或不接纳该性别  -3=房间住客性别与钦定值冲突
    int set_dorm_gender(int building_id, int dorm_id, int gender);
	int set_dorm_max_num(int building_id, int dorm_id, int max_num);//修改宿舍最大人数, 1=成功/0=宿舍不存在/-1=参数非法/-2=缩容丢人
	int add_student_to_dorm(int building_id, int dorm_id, int student_id);//入住指定宿舍, -8=宿舍不存在, 其它透传 dorm::add_student
	int add_student_to_dorm(int building_id, int dorm_id, int student_id, int bed_id);//入住指定床位, -8=宿舍不存在, 其它透传 dorm::add_student
	int add_student_to_available_dorm(int student_id);//入住最小顺位可用宿舍, -9=无可用宿舍, 其它透传 dorm::add_student
	int add_student_to_available_dorm_random(int student_id);//随机入住可用宿舍, -9=无可用宿舍, 其它透传 dorm::add_student

    //按楼号、宿舍号取本体(只读)。不存在返回 nullptr。
	//注意: 返回指针指向 QMap 内部。QMap 为红黑树, 插入不会使已有项引用失效; 但删除被指向的项后指针失效。
	//请就地使用, 不要长期持有; 需长期引用请存宿舍楼号, 用时再 get。
    const dorm* get(int building_id, int dorm_id) const;
    const dorm* get_available_dorm(int gender);//获取指定性别可用的宿舍（默认最小可用楼号中的最小可用宿舍号）
    const dorm* get_available_dorm_random(int gender);//获取指定性别可用的宿舍（随机选择）

    //逻辑判断
    int is_dorm_exist(int building_id, int dorm_id);//检查指定楼号、宿舍号是否存在，约定返回值-1为楼号不存在，0为宿舍号不存在，1为存在

    //高级信息查询
    int count() const;//获取当前宿舍总数。获取指定楼号的宿舍数等功能可调用get()通过building类的方法实现
    int get_empty_count() const;//获取当前空宿舍总数
    int get_occupied_count() const;//获取当前已占用宿舍总数

private:
	dormmanager() = default;//构造函数, 单例模式禁止外部实例化
	QMap<int, QMap<int, dorm>> dorms;//宿舍本体有序表, 外层key为宿舍楼号, value为该楼的宿舍本体有序表, 内层key为宿舍号, value为宿舍本体。QMap按key升序, 遍历天然按楼号、宿舍号顺序
};

#endif // DORMMANAGER_H
