#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class OverviewPage;
}
QT_END_NAMESPACE

//数据概览页面。负责展示学校协调层提供的只读统计结果。
class OverviewPage : public QWidget
{
	Q_OBJECT

public:
	explicit OverviewPage(QWidget* parent = nullptr);
	~OverviewPage();

private:
	Ui::OverviewPage* ui;
};

#endif // OVERVIEWPAGE_H
