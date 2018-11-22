#include "mainwindow.h"
#include <QApplication>
#include <locale>

int main(int argc, char *argv[])
{
	std::setlocale(LC_NUMERIC, "en_US.UTF-8");
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
