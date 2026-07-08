#ifndef DORMMANAGER_H
#define DORMMANAGER_H

#include "dorm.h"
#include <QVector>
#include <QHash>

// 全校宿舍本体的唯一归属。以宿舍编号(id)为 key 存储 dorm 本体, 平均 O(1) 按宿舍编号查找。
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

    //按楼号、宿舍号取本体(可修改)。不存在返回 nullptr。
	//注意: 返回指针指向 QHash 内部, 在后续 add/remove 触发 rehash 后可能失效。
	//请就地使用, 不要长期持有; 需长期引用请存宿舍楼号, 用时再 get。
    dorm* get(int building_id, int dorm_id);
    const dorm* get(int building_id, int dorm_id) const;//const 重载, 返回只读指针
    int count() const;//获取当前宿舍总数

    //逻辑判断
    int is_dorm_exist(int building_id, int dorm_id);//检查指定楼号、宿舍号是否存在，约定返回值-1为楼号不存在，0为宿舍号不存在，1为存在

private:
	dormmanager() = default;//构造函数, 单例模式禁止外部实例化
	QHash<int, QHash<int, dorm>> dorms;//宿舍本体哈希表, key为宿舍楼号, value为宿舍本体哈希表, key为宿舍楼层编号, value为宿舍本体   
};

#endif // DORMMANAGER_H