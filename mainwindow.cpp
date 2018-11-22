#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTimer>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <list>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	retrySerialPort();
	timer = new QTimer(this);
	computationTimer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(useSerialPort()));
	connect(computationTimer, SIGNAL(timeout()), this, SLOT(compute()));

	if (prefs.simulationFile.empty()) {
		ui->simulationFileEdit->hide();
		ui->simulationFileLabel->hide();
	}

	ui->portEdit->setText(QString::fromStdString(prefs.port));
	ui->baudrateEdit->setText(QString::fromStdString(std::to_string(prefs.baudrate)));
	ui->simulationFileEdit->setText(QString::fromStdString(prefs.simulationFile));
	ui->readingIntervalEdit->setText(QString::fromStdString(std::to_string(prefs.readingInterval)));
	ui->massEdit->setText(QString::fromStdString(std::to_string(prefs.mass)));
	ui->solidDensityEdit->setText(QString::fromStdString(std::to_string(prefs.solidDensity)));
	ui->liquidDensityEdit->setText(QString::fromStdString(std::to_string(prefs.liquidDensity)));
	ui->computationIntervalEdit->setText(QString::fromStdString(std::to_string(prefs.computationInterval)));
	ui->filterMinEdit->setText(QString::fromStdString(std::to_string(prefs.filterMin)));
	ui->filterMaxEdit->setText(QString::fromStdString(std::to_string(prefs.filterMax)));

	ui->plot->xAxis->setLabel("Time [s]");
	ui->plot->yAxis->setLabel("Mass [g]");
}

MainWindow::~MainWindow()
{
	prefs.save();
	delete ui;
}

void MainWindow::readInput() {
	prefs.baudrate = std::stoi(ui->baudrateEdit->text().toStdString());
	prefs.port = ui->portEdit->text().toStdString();
	prefs.readingInterval = std::stoi(ui->readingIntervalEdit->text().toStdString());
	prefs.simulationFile = ui->simulationFileEdit->text().toStdString();
	prefs.mass = std::stof(ui->massEdit->text().toStdString());
	prefs.solidDensity = std::stof(ui->solidDensityEdit->text().toStdString());
	prefs.liquidDensity = std::stof(ui->liquidDensityEdit->text().toStdString());
	prefs.computationInterval = std::stoi(ui->computationIntervalEdit->text().toStdString());
	prefs.filterMin = std::stof(ui->filterMinEdit->text().toStdString());
	prefs.filterMax = std::stof(ui->filterMaxEdit->text().toStdString());
}

void MainWindow::retrySerialPort() {
	serialPort.setBaudRate(prefs.baudrate);
	serialPort.setPortName(QString::fromStdString(prefs.port));
	serialPort.setStopBits(QSerialPort::OneStop);
	serialPort.setParity(QSerialPort::Parity::NoParity);
	serialPortOpen = serialPort.open(QIODevice::ReadOnly);
}

void MainWindow::useSerialPort() {
	std::string input;
	if (!serialPortOpen)
		retrySerialPort();
	if (!serialPortOpen) {
		if (prefs.simulationFile.empty()) {
			ui->statusBar->showMessage("Could not connect to port");
			stop();
			return;
		}
		if (!simulationFile)
			simulationFile = std::make_unique<std::ifstream>(prefs.simulationFile);
		if (!simulationFile->good()) {
			ui->statusBar->showMessage("Done reading");
			stop();
			return;
		}
		std::getline(*simulationFile, input, '\n');
		ui->statusBar->showMessage("Simulation running...");
		//std::cout << input << std::endl;
	} else {
		ui->statusBar->showMessage("Measurement running...");
		input = serialPort.readAll().toStdString();
	}
	if (input.empty()) return;

	// We get here only if we managed to read something
	std::stringstream inputStream(input);
	std::string line;
	while (std::getline(inputStream, line, '\n')) {
		float got = 0;
		int sign = 1;
		int at = 0;
		for ( ; line[at] && line[at] != '.'; at++) {
		  if (line[at] >= '0' && line[at] <= '9') {
			got *= 10;
			got += line[at] - '0';
		  } else if (line[at] == '-') {
			sign = -1;
		  }
		}
		at++;
		float behind = 0.1;
		for ( ; line[at]; at++) {
		  if (line[at] >= '0' && line[at] <= '9') {
			got += behind * (line[at] - '0');
			behind /= 10;
		  }
		}
		got *= sign;
		data.push_back(got);
	}
	ended = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	plot();
}

void MainWindow::start() {
	readInput();
	simulationFile.reset();
	data.clear();
	timer->start(prefs.readingInterval);
	computationTimer->start(prefs.computationInterval);
	running = true;
	ui->startStopButton->setText("Stop");
	started = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void MainWindow::stop() {
	timer->stop();
	computationTimer->stop();
	running = false;
	ui->startStopButton->setText("Start");
	compute();
	plot();
}

float MainWindow::getAuxiliaryTimeInterval() {
	return (ended - started) / ((float)data.size() * 1000);
}

void MainWindow::plot() {
	ui->plot->clearGraphs();
	ui->plot->addGraph();
	ui->plot->graph(0)->setPen(QPen(Qt::blue));
	float timeInterval = getAuxiliaryTimeInterval();
	for (unsigned int i = 0; i < data.size(); i++) {
		ui->plot->graph(0)->addData(i * timeInterval, data[i]);
	}
	if (increaseStarted > 0 && aFit == aFit && bFit == bFit) {
		ui->plot->addGraph();
		ui->plot->graph(1)->setPen(QPen(Qt::green));
		int end = std::max(matchEnd, (int)data.size());
		for (int i = increaseStarted; i < end; i++) {
			ui->plot->graph(1)->addData(i * timeInterval, functionAt((i - increaseStarted) * timeInterval));
		}
	}
	ui->plot->setInteraction(QCP::iRangeDrag, true);
	ui->plot->setInteraction(QCP::iRangeZoom, true);
	ui->plot->rescaleAxes();
	ui->plot->replot();
}

float MainWindow::functionAt(float x) {
	float m = (-aFit + bFit * y0) / aFit * exp(-1 + bFit / aFit * (bFit * (x0 - x) + y0));
	float twoEM = 2 * 2.71828 * m;
	float rt = sqrt(2 + twoEM);
	float y = aFit / bFit * (1 + (twoEM + 7.56859 * rt - 10.7036) / (5.13501 * rt + 12.7036));
	return y;
}

void MainWindow::on_exitButton_clicked()
{
	readInput();
	close();
}

void MainWindow::on_startStopButton_clicked()
{
	if (running) stop();
	else start();
}

void MainWindow::on_saveButton_clicked()
{
	QString QFileName = QFileDialog::getSaveFileName(this, tr("Save Data"), QString::fromStdString(prefs.folder), tr("Files (*.txt)"));
	if (!QFileName.size()) return;
	std::string fileName = QFileName.toStdString();
	prefs.folder = fileName.substr(0, fileName.find_last_of("/\\"));
	std::ofstream out(fileName);
	float timeInterval = getAuxiliaryTimeInterval();
	for (unsigned int i = 0; i < data.size(); i++) {
		out << (timeInterval * i) << "\t" << data[i] << std::endl;
	}
}

void MainWindow::compute() {
	massZero = NAN;
	massMax = NAN;
	massClean = NAN;
	aFit = NAN;
	bFit = NAN;
	x0 = NAN;
	y0 = NAN;
	increaseStarted = -1;
	matchEnd = -1;

	vTotal = NAN;
	v1 = prefs.mass / prefs.solidDensity;
	v2 = NAN;
	porousness = NAN;
	alpha = NAN;
	bigA = NAN;
	bigB = NAN;
	mMax = NAN;

	if (data.size() == 0) return;
	massZero = data[0];
	float averageTotal = 0;
	std::list<float> numbers;
	unsigned int index = 0;
	bool done = false;
	auto checkRollingAverage = [&] (bool includeUnstable) {
		//std::cout << "Rolling average " << (averageTotal / numbers.size());
		if (numbers.size() < (unsigned int)prefs.stabilityLength) {
			numbers.push_front(data[index]);
			averageTotal += data[index];
			//std::cout << " no deviation " << std::endl;
		} else {
			float deviation = data[index] - (averageTotal / numbers.size());
			if (deviation < 0) deviation *= -1;
			float realSum = 0;
			for (float it : numbers) realSum += it;
			//std::cout << " deviation " << data[index] << " - " << (averageTotal / numbers.size()) << " or " << (realSum / numbers.size()) << " vs " << prefs.constantInstability << std::endl;
			numbers.push_front(data[index]);
			if (deviation > prefs.constantInstability) {
				if (includeUnstable) {
					averageTotal += data[index] - numbers.back();
					numbers.pop_back();
				} else numbers.pop_front();
				return true;
			} else {
				averageTotal += data[index] - numbers.back();
				numbers.pop_back();
			}
		}
		return false;
	};

	// Initial part
	int instability = 0;
	while (index < data.size()) {
		massZero = averageTotal / numbers.size();
		if (checkRollingAverage(false)) instability++;
		else instability = std::max(instability - 1, 0);
		if (instability > prefs.instabilityTolerance) break;
		index++;
	}
	std::cout << "initial part ends at " << index << std::endl;
	if (index < data.size()) {

		// Increasing part
		increaseStarted = index - 1;
		instability = -prefs.increaseEndStability;
		while (index < data.size()) {
			if (checkRollingAverage(true)) instability = std::max(instability - 1,  -prefs.increaseEndStability);
			else instability++;
			if (instability >= 0) break;
			index++;
		}
		int increaseEnded = index - 1;
		std::cout << "increasing range " << increaseStarted << " " << increaseEnded << std::endl;

		double interestingAverage = 0;
		int interestingPoints = 0;
		int interestingEnd = increaseEnded;
		instability = 0;
		for (int i = increaseStarted + 1; i < increaseEnded; i++) {
			float derivative = data[i] - data[i - 1];
			interestingAverage += derivative;
			if (interestingPoints >= prefs.increaseEndStability) {
				interestingAverage -= data[i - interestingPoints] - data[i - interestingPoints - 1];
				float deviation = derivative - interestingAverage / interestingPoints;
				if (deviation < 0) deviation *= -1;
				interestingEnd = i;
				if (deviation < prefs.bendEndDeviation) instability++;
				else instability = std::max(instability - 1, 0);
				if (instability > prefs.instabilityTolerance) break;
			} else interestingPoints++;
		}
		instability = 0;
		for (int i = interestingEnd + increaseStarted + 1; i < increaseEnded; i++) {
			float derivative = data[i] - data[i - 1];
			interestingAverage += derivative;
			interestingAverage -= data[i - interestingPoints] - data[i - interestingPoints - 1];
			float deviation = derivative - interestingAverage / interestingPoints;
			if (deviation < 0) deviation *= -1;
			interestingEnd = i - interestingPoints;
			if (deviation > prefs.bendEndDeviation) instability++;
			else instability = std::max(instability - 1, 0);
			if (instability > prefs.instabilityTolerance) break;
		}


		std::cout << "Interesting points end at " << interestingEnd << std::endl;

		std::vector<std::pair<float, float>> representatives;
		for (int i = increaseStarted; i < interestingEnd; i += std::max((interestingEnd - increaseStarted) / prefs.representatives, 1)) {
			representatives.push_back(std::make_pair((i - increaseStarted) * getAuxiliaryTimeInterval(), data[i]));
		}
		auto meanDeviation = [&] () -> float {
			float result = 0;
			for (unsigned int i = 0; i < representatives.size(); i++) {
				float yDiv = functionAt(representatives[i].first) - representatives[i].second;
				result += yDiv * yDiv;
			}
			return result;
		};
		float aStep = prefs.aStep;
		float bStep = prefs.bStep;
		float x0step = prefs.x0step;
		float y0step = prefs.y0step;
		aFit = prefs.startingA;
		bFit = prefs.startingB;
		x0 = prefs.startingX0;
		y0 = prefs.startingY0;
		for (int repetition = 0; repetition < prefs.stepReductions; repetition++) {
			bool didSomething = true;
			float finalBest = 9000000000;
			while (didSomething) {
				didSomething = false;
				std::array<float, 3> possibilities = {aFit - aStep, aFit, aFit + aStep};
				auto tryPossibilities = [&] (float& change) {
					float best = 9000000000000;
					int bestAt = 1;
					for (int i = 0; i < 3; i++) {
						change = possibilities[i];
						float got = meanDeviation();
						if (got < best) {
							best = got;
							bestAt = i;
						}
					}
					if (best < finalBest) finalBest = best;
					if (bestAt != 1) didSomething = true;
					change = possibilities[bestAt];
				};
				tryPossibilities(aFit);
				possibilities = {bFit - bStep, bFit, bFit + bStep};
				tryPossibilities(bFit);
				possibilities = {x0 - x0step, x0, x0 + x0step};
				tryPossibilities(x0);
				possibilities = {y0 - y0step, y0, y0 + y0step};
				tryPossibilities(y0);
			}

			aStep /= prefs.stepReductionDivision;
			bStep /= prefs.stepReductionDivision;
			x0step /= prefs.stepReductionDivision;
			y0step /= prefs.stepReductionDivision;
		}
		std::cout << "Computed values a " << aFit << " b " << bFit << " x0 " << x0 << " y0 " << y0 << std::endl;

		mMax = aFit / bFit;

		std::cout << "increasing part ends at " << index << std::endl;
		if (index < data.size()) {

			// Saturated part
			instability = 0;
			while (index < data.size()) {
				massMax = averageTotal / numbers.size();
				if (checkRollingAverage(false))instability++;
				else instability = std::max(instability - 1, 0);
				if (instability > prefs.instabilityTolerance) break;
				index++;
			}
			std::cout << "saturated part ends at " << index << std::endl;
			if (index < data.size()) {

				// Swing
				while (index < data.size() && instability > 0) {
					if (checkRollingAverage(true)) instability++;
					else instability = std::max(instability - 1, 0);
					index++;
				}
				std::cout << "swing part ends at " << index << std::endl;
				if (index < data.size()) {
					massClean = averageTotal / numbers.size();
					v2 = massClean / prefs.liquidDensity;
					vTotal = v1 + v2;
					porousness = v2 / (v1 + v2);
					alpha = (massMax - prefs.filterMax - massClean + prefs.filterMin) / (massClean - prefs.filterMin);
					bigA = aFit / (pow(porousness, 4) / ((1 - porousness) * (1 - porousness)));
					bigA = bFit / (pow(porousness, 3) / ((1 - porousness) * (1 - porousness)));
					porousness *= 100;
					done = true;
				}
			}
		}
	}

	// Print the resuts
	std::string result;
	std::stringstream out(result);
	out << "Computed values: ";
	auto notNan = [] (float x) -> bool { return (x == x); };
	if (notNan(massZero)) out << "mass at zero: " << massZero << ", ";
	if (notNan(massMax)) out << "mass at maximum: " << massMax << ", ";
	if (notNan(massClean)) out << "mass when detached: " << massClean << ", ";
	if (notNan(aFit)) out << "parameter a: " << aFit << ", ";
	if (notNan(bFit)) out << "parameter b: " << bFit << ", ";
	if (notNan(vTotal)) out << "total volume: " << vTotal << ", ";
	if (notNan(v1)) out << "volume of solid: " << v1 << ", ";
	if (notNan(v2)) out << "volume of liquid: " << v2 << ", ";
	if (notNan(porousness)) out << "porousness: " << porousness << "%, ";
	if (notNan(alpha)) out << "alpha: " << alpha << ", ";
	if (notNan(bigA)) out << "parameter A: " << bigA << ", ";
	if (notNan(bigB)) out << "parameter B: " << bigB << ", ";
	if (notNan(mMax)) out << "maximal m: " << mMax << ", ";
	if (done) out << "all data obtained.";
	else out << "still measuring.";

	ui->outputLabel->setText(QString::fromStdString(out.str()));
}
