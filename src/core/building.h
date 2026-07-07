#ifndef BUILDING_H
#define BUILDING_H

#include <QVector>

class building
{
public:
	building();

private:
	int id;//宿舍楼号
	int max_floor;//最大楼层数
	QVector<QVector<int>> dorm_ids;//每层的宿舍号列表(二维数组，第一维为楼层号下标(自然数-1)，第二维为该楼层的宿舍号列表)
};

#endif // BUILDING_H