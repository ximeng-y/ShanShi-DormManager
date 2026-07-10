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

	//===== 空床位总数统计(注意与已有 get_empty_count()=空宿舍数区分) =====
	//无参: 全校空床位总数。带 gender(1/2): 该性别可用空床(纯性别楼对号入座, 混宿楼按房间锁, 未锁定房算入该性别)。
	//带 building_id: 限定某楼。楼未注册于 buildingmanager 时该楼跳过(与 get_available_dorm 一致)。
	//返回值: >=0=床位数  -1=参数非法(gender不为1/2, 或 building_id 格式错)
	int get_empty_bed_count() const;
	int get_empty_bed_count(int gender) const;
	int get_empty_bed_count_of_building(int building_id) const;
	int get_empty_bed_count_of_building(int building_id, int gender) const;

	//===== 清空所有宿舍(同步重置学生位置字段) =====
	//clear_all_dorms 保留各房间性别锁; clear_all_dorms_reset_gender 额外放开所有房间性别锁。
	//返回值: 被清退的学生总数(>=0)
	int clear_all_dorms();
	int clear_all_dorms_reset_gender();

	//===== 两间宿舍整体调换及其两种善后策略 =====
	//整体调换: 要求两间性别锁(for_gender)相同且实际人数相同, 满足则整体互换住客(自动分床)。
	//返回值: 1=成功  -1=参数非法(id格式错或两参指向同一间)  -2=某间不存在  -3=两间性别锁不同(无法调换)  -4=性别相同但人数不同(顶层可据此让用户在下面两个善后策略中选择)
	int swap_dorms(int b1, int d1, int b2, int d2);
	//重叠床位互换(善后策略B): 性别锁须相同; 取 k=两间人数较小值, 各交换前 k 人, 人多一方多余的人留在原宿舍原位。
	//返回值: 1=成功(含一方为空的无操作)  -1=参数非法  -2=某间不存在  -3=两间性别锁不同
	int swap_dorms_overlap(int b1, int d1, int b2, int d2);
	//重叠床位互换 + 多余离宿(善后策略C): 同上取 k, 前 k 人交叉入住, 人多一方多余的人进入无宿舍状态。
	//返回值: 1=成功  -1=参数非法  -2=某间不存在  -3=两间性别锁不同
	int swap_dorms_overlap_evict(int b1, int d1, int b2, int d2);

	//===== 混合楼间一对男舍↔女舍整体互换 =====
	//前提: 两间所在楼都必须是混宿楼(building.for_gender==3), 且两间恰为一男舍一女舍。
	//人数一致则整体互换; 不一致则前 k=较小人数 交叉互换、人多一方多余的人进入无宿舍状态。
	//返回值: 1=成功  -1=参数非法  -2=某间不存在  -3=有楼未注册或非混宿楼  -4=两间不是一男一女
	int swap_gender_dorms(int b1, int d1, int b2, int d2);

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
	//仅供 school 完成两端同步闭环的床位写接口，不得由普通调用者直接使用。
	int add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender);//向指定宿舍写入学生学号, 不查询或同步学生本体, -8=宿舍不存在
	int add_student_to_dorm(int building_id, int dorm_id, int student_id, int gender, int bed_id);//向指定床位写入学生学号, 不查询或同步学生本体, -8=宿舍不存在
	int remove_student_from_dorm(int building_id, int dorm_id, int student_id);//从指定宿舍移除学生学号, 不同步学生本体, -8=宿舍不存在
	friend class school;
	//内部定位: 按楼号、宿舍号返回可写本体指针(供内部搬迁改写用), 不存在返回 nullptr。
	//对外仍只暴露 const get(); 本 helper 不校验 id 格式(调用方负责)。
	dorm* find_dorm(int building_id, int dorm_id);
	//交换前公共校验: 参数合法性、非同一间、两间均存在、性别锁相同。
	//通过后 A/B 指向两间可写本体; 失败返回负值错误码。
	//返回值: 0=成功  -1=参数非法或指向同一间  -2=某间不存在  -3=性别锁不同
	int validate_swap_pair(int b1, int d1, int b2, int d2, dorm*& A, dorm*& B);
	//清空所有宿舍内部实现, reset_gender 控制是否同时放开房间性别锁
	int clear_all_dorms_impl(bool reset_gender);
	//临时同步 helper：dorm 已不再依赖 studentmanager，在 school 接管协调前由本类维持学生位置字段一致性。
	void sync_dorm_students(const dorm& d);
	void reset_and_sync_students(const QVector<int>& original_ids, const dorm& A, const dorm& B);
	QMap<int, QMap<int, dorm>> dorms;//宿舍本体有序表, 外层key为宿舍楼号, value为该楼的宿舍本体有序表, 内层key为宿舍号, value为宿舍本体。QMap按key升序, 遍历天然按楼号、宿舍号顺序
};

#endif // DORMMANAGER_H
