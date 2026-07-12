#ifndef APPSTYLE_H
#define APPSTYLE_H

#include <QPalette>
#include <QString>

//应用统一样式入口。仅提供界面视觉规范，不持有业务状态。
class appstyle
{
public:
	static QPalette light_palette();//返回不受系统深色模式影响的完整浅色调色板
	static QString stylesheet();//返回应用级 Qt Style Sheet
};

#endif // APPSTYLE_H
