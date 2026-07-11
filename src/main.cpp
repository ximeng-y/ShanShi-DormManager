#include "ui/mainwidget.h"
#include "ui/appstyle.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	a.setStyle(QStringLiteral("Fusion"));
	a.setStyleSheet(appstyle::stylesheet());
    MainWidget w;
    w.show();
    return a.exec();
}
