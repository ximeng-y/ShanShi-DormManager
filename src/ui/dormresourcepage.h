#ifndef DORMRESOURCEPAGE_H
#define DORMRESOURCEPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class DormResourcePage;
}
QT_END_NAMESPACE

//宿舍资源页面。以楼栋、宿舍、床位三级结构展示住宿空间。
class DormResourcePage : public QWidget
{
	Q_OBJECT

public:
	explicit DormResourcePage(QWidget* parent = nullptr);
	~DormResourcePage();

private:
	Ui::DormResourcePage* ui;
};

#endif // DORMRESOURCEPAGE_H
