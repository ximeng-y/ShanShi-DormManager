#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QString>

//应用统一样式入口。仅提供界面视觉规范，不持有业务状态。
class appstyle
{
public:
	static QString stylesheet();//返回应用级 Qt Style Sheet
};

#endif // APPSTYLE_H
