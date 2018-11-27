#include "mainwindow.h"
#include <QApplication>
#include <locale>

// This work of art is dedicated to Martina Ilčíková who explained me all the theory about this

int main(int argc, char *argv[])
{
	std::setlocale(LC_NUMERIC, "en_US.UTF-8");
	QApplication a(argc, argv);
	MainWindow w;
	w.show();

	return a.exec();
}
