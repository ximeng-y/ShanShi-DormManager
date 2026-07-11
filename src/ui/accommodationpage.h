#ifndef ACCOMMODATIONPAGE_H
#define ACCOMMODATIONPAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class AccommodationPage;
}
QT_END_NAMESPACE

//住宿安排页面。按任务组织入住、退宿、调宿换床与学生互换流程。
class AccommodationPage : public QWidget
{
	Q_OBJECT

public:
	explicit AccommodationPage(QWidget* parent = nullptr);
	~AccommodationPage();

private:
	Ui::AccommodationPage* ui;
};

#endif // ACCOMMODATIONPAGE_H
