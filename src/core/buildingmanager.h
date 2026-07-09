#ifndef BUILDINGMANAGER_H
#define BUILDINGMANAGER_H

#include <QMap>
#include "building.h"

//全校宿舍楼本体的唯一归属。以宿舍楼号(id)为 key 存储 building 本体, 按楼号升序有序存储。
//dorm/协调层等只存宿舍楼号, 通过本类换取本体, 保证单一数据源、无副本一致性问题。
class buildingmanager
{
public:
	static buildingmanager& instance();//创建单例入口
	buildingmanager(const buildingmanager&) = delete;//禁止拷贝构造，维护单例唯一性
	buildingmanager& operator=(const buildingmanager&) = delete;//禁止拷贝赋值，维护单例唯一性

	//操作宿舍楼
	//添加宿舍楼: 校验字段合法(building_id/for_gender/max_floor)且楼号唯一
	//返回值: true=成功  false=字段非法或楼号已存在
	bool add_building(const building& b);
	//移除宿舍楼: 仅从表中删除, 不级联清理楼内宿舍(跨 manager 协调留待上层 school)
	//返回值: true=成功  false=楼号不存在
	bool remove_building(int building_id);

	//按楼号取本体(只读)。不存在返回 nullptr。
	//注意: 返回指针指向 QMap 内部。QMap 为红黑树, 插入不会使已有项引用失效; 但删除被指向的项后指针失效。
	//请就地使用, 不要长期持有; 需长期引用请存宿舍楼号, 用时再 get。
	const building* get(int building_id) const;

	//逻辑判断 / 查询
	int is_building_exist(int building_id);//检查指定楼号是否存在, 1=存在/0=不存在/-1=参数非法
	int count() const;//获取当前宿舍楼总数

private:
	buildingmanager() = default;//构造函数, 单例模式禁止外部实例化
	QMap<int, building> buildings;//宿舍楼本体有序表, key为宿舍楼号, value为宿舍楼本体
};

#endif // BUILDINGMANAGER_H
