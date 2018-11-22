# washburn
This is a program to read data from a balance through a serial port and compute results of Washburn experiment.

## Setting up
You need Qt to compile it. Although the parser is quite robust, it might not work with your balance. If that happens, please make a pull request.

## Usage
Set up the communication parametres or try the default ones, then hit the _Start_ button. It will make the measurement. Hit stop to end the experiment.

A lot of data is computed from experimental conditions set below the graph. If you don't know what those results are, feel free to ignore them.

## Advanced
Some more advanced settings can be set manually in file _washburn.ini_ and it's the only way to change them. You may use them to optimise the program to process data from your experiment better. Use the _simulation_file_ setting to read from a file rather than from the serial port.
