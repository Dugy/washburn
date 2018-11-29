#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSerialPort/QSerialPort>
#include "preferences.h"
#include <memory>
#include <cmath>

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private slots:
	void useSerialPort();
	void compute();

	void on_exitButton_clicked();

	void on_startStopButton_clicked();

	void on_saveDataButton_clicked();

	void on_setResultsFileButton_clicked();

	void on_saveResultsButton_clicked();

private:
	Ui::MainWindow* ui;
	QTimer* timer;
	QTimer* computationTimer;

	QSerialPort serialPort;
	bool serialPortOpen;
	preferences prefs;
	std::unique_ptr<std::ifstream> simulationFile;

	std::vector<float> data;
	uint64_t started;
	uint64_t ended;

	void start();
	void stop(bool hideMessage = false);
	bool running = false;
	bool usingFile = false;

	float getAuxiliaryTimeInterval();
	void retrySerialPort();
	void plot();
	void readInput();
	float functionAt(float x);

	// From the file in Origin: m=(-a+b*y0)/a*exp(-1+b/a*(b*(x0-x)+y0)); y=a/b*(1+(2*e*m + 7.56859*sqrt(2+2*e*m)-10.7036)/(5.13501*sqrt(2+2*e*m) + 12.7036))
	// Rewritten as: m = (-1 + b*y0/a) * exp(-1+b^2/a*x0+b*y0/a-x*b^2/a)
	float massZero = NAN;
	float massMax = NAN;
	float massClean = NAN;
	float aFit = NAN;
	float bFit = NAN;
	float x0 = NAN;
	float y0 = NAN;
	int increaseStarted = -1;
	int matchEnd = -1;

	float vTotal = NAN;
	float v1 = NAN;
	float v2 = NAN;
	float porousness = NAN;
	float alpha = NAN;
	float bigA = NAN;
	float bigB = NAN;
	float mMax = NAN;
};

#endif // MAINWINDOW_H
