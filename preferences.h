#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <string>
#include <functional>

struct preferences
{
	preferences();
	int baudrate;
	std::string port;
	std::string folder;
	int readingInterval;
	std::string simulationFile;
	float mass;
	float solidDensity;
	float liquidDensity;
	float filterMin;
	float filterMax;
	int computationInterval;
	float constantInstability;
	int stabilityLength;
	int instabilityTolerance;
	int increaseEndStability;
	float startingA;
	float aStep;
	float startingB;
	float bStep;
	float startingX0;
	float x0step;
	float startingY0;
	float y0step;
	int stepReductions;
	float stepReductionDivision;
	int representatives;
	float bendEndDeviation;

	void load();
	void save();
	void setup();

	void syncHelper(std::function<void(const std::string&, int&)> intFunc,
					std::function<void(const std::string&, float&)> floatFunc,
			  std::function<void(const std::string&, std::string&)> stringFunc) {
		intFunc("baud_rate", baudrate);
		stringFunc("port", port);
		stringFunc("folder", folder);
		intFunc("reading_interval", readingInterval);
		stringFunc("simulation_file", simulationFile);
		floatFunc("mass", mass);
		floatFunc("liquid_density", liquidDensity);
		floatFunc("solid_density", solidDensity);
		floatFunc("filter_min", filterMin);
		floatFunc("filter_max", filterMax);
		intFunc("computation_interval", computationInterval);
		floatFunc("constant_instability", constantInstability);
		intFunc("stability_length", stabilityLength);
		intFunc("instability_tolerance", instabilityTolerance);
		intFunc("increase_end_stability", increaseEndStability);
		floatFunc("starting_a", startingA);
		floatFunc("a_step", aStep);
		floatFunc("starting_b", startingB);
		floatFunc("b_step", bStep);
		floatFunc("starting_x0", startingX0);
		floatFunc("x0_step", x0step);
		floatFunc("starting_y0", startingY0);
		floatFunc("y0_step", y0step);
		intFunc("step_reductions", stepReductions);
		floatFunc("step_reduction_division", stepReductionDivision);
		intFunc("representatives", representatives);
		floatFunc("bend_end_deviation", bendEndDeviation);
	}
};

#endif // PREFERENCES_H
