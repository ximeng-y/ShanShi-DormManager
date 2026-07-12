#include "ui/mainwidget.h"
#include "ui/appstyle.h"
#include "ui/windowtheme.h"
#include "core/school.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setStyle(QStringLiteral("Fusion"));
	a.setPalette(appstyle::light_palette());
	a.setStyleSheet(appstyle::stylesheet());
	windowtheme::install(a);
	school::instance().initialize_persistence();
    MainWidget w;
    w.show();
    return a.exec();
}
