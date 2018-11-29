#include "preferences.h"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <cmath>

inline float stor(const char* str) {
	// The problem with std::stof and atof is that it's locale dependent and unpredictably
	// accepts decimal dot or decimal comma and rejects the other, this one accepts both,
	// the number parser in formula accepts both too
	float result = 0;
	float sign = *str == '-' ? str++, -1 : 1;
	while (*str >= '0' && *str <= '9') {
		result *= 10;
		result += *str - '0';
		str++;
	}
	if (*str == ',' || *str == '.') {
		str++;
		float multiplier = 0.1;
		while (*str >= '0' && *str <= '9') {
			result += (*str - '0') * multiplier;
			multiplier /= 10;
			str++;
		}
	}
	result *= sign;
	if (*str == 'e' || *str == 'E') {
		str++;
		float powerer = *str == '-'? str++, 0.1 : 10;
		float power = 0;
		while (*str >= '0' && *str <= '9') {
			power *= 10;
			power += *str - '0';
			str++;
		}
		result *= pow(powerer, power);
	}
	return result;
}

preferences::preferences()
{
	load();
}

void preferences::setup() {
	baudrate = 9600;
	port = "COM1";
	folder = "";
	readingInterval = 100;
	simulationFile = "";
	mass = 2;
	solidDensity = 1.5;
	liquidDensity = 1;
	filterMin = 0.3565;
	filterMax = 0.5367;
	computationInterval = 1000;
	stabilityLength = 20;
	instabilityTolerance = 3;
	increaseEndStability = 20;
	startingA = 5;
	aStep = 2;
	startingB = 0.01;
	bStep = 0.01;
	startingX0 = 0;
	x0step = 1;
	startingY0 = 0.2;
	y0step = 0.1;
	stepReductions = 10;
	stepReductionDivision = 2;
	representatives = 10;
	bendEndDeviation = 0.01;
	maximumLoops = 1000000;
}

void preferences::load() {
	std::ifstream in("washburn.ini");
	try {
		if (in.good()) {
			std::string line;
			std::unordered_map<std::string, std::string> parsed;
			while (std::getline(in, line)) {
				auto separator = line.find('=');
				parsed[line.substr(0, separator)] = line.substr(separator + 1);
			}
			syncHelper([&] (const std::string& name, int& value) {
				value = std::stoi(parsed[name]);
			}, [&] (const std::string& name, float& value) {
				value = stor(parsed[name].c_str());
			}, [&] (const std::string& name, std::string& value) {
				value = parsed[name];
			});
		} else setup();
	} catch (std::exception& e) {
		setup();
	}
}

void preferences::save() {
	std::ofstream out("washburn.ini");
	syncHelper([&] (const std::string& name, int& value) {
		out << name << "=" << value << std::endl;
	}, [&] (const std::string& name, float& value) {
		out << name << "=" << value << std::endl;
	}, [&] (const std::string& name, std::string& value) {
		out << name << "=" << value << std::endl;
	});
}
