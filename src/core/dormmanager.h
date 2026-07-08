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

    bool add_dorm(const dorm& dorm);//添加宿舍
    bool remove_dorm(int dorm_id);//删除宿舍

private:
	dormmanager() = default;//构造函数, 单例模式禁止外部实例化
	QHash<int, QHash<int, dorm>> dorms;//宿舍本体哈希表, key为宿舍楼号, value为宿舍本体哈希表, key为宿舍楼层编号, value为宿舍本体   
};

#endif // DORMMANAGER_H