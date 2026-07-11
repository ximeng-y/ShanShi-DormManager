#include "accommodationpage.h"
#include "./ui_accommodationpage.h"

AccommodationPage::AccommodationPage(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui::AccommodationPage)
{
	ui->setupUi(this);
}

AccommodationPage::~AccommodationPage()
{
	delete ui;
}
