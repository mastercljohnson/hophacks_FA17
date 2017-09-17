// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.

// This sample illustrates how to use EMG data. EMG streaming is only supported for one Myo at a time.

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <fstream>
#include <chrono>
#include <gsl/gsl_fit.h>
#include <myo/myo.hpp>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <array>
#include <fstream>
#include <time.h>


class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		:emgSamples()
	{
	}
	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void extract(std::ifstream emg_data_test) {
		int extractarray[8][50];

		for (int row = 0; row < 8; ++row)
		{
			std::string line;
			std::getline(emg_data_test, line);
			if (!emg_data_test.good())
				break;
			std::stringstream iss(line);

			for (int col = 0; col < 50; ++col)
			{
				std::string val;
			std:getline(iss, val, ',');
				if (!iss.good())
					break;

				std::stringstream converter(val);
				converter >> extractarray[row][col];
			}
		}
	}
	std::array<int8_t, 8> print()
	{

		// Clear the current line
		std::cout << '\r';
		

		// Print out the EMG data.
		/*
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();


			std::cout << '[' << emgString << std::string(4 - emgString.size(), ' ') << ']';
		}*/

		
		std::cout << std::flush;
		return emgSamples;
	}

	// The values of this array is set by onEmgData() above.
	
	std::array<int8_t, 8> emgSamples;

};



int main(int argc, char** argv)
{
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {
		double most_recent = -1000000.0000000;
		double previous_gradient = -1000000.0000000;



		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.example.emg-data-sample");

		std::cout << "Attempting to find a Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		myo::Myo* myo = hub.waitForMyo(10000);

		// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}

		// We've found a Myo.
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

		// Next we enable EMG streaming on the found Myo.
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);
		int cycles = 0;
		//counter until next RMS
		std::array<std::array<int8_t, 8>, 25> samples;
		int minute_counter = 0;
		std::array <std::array<double, 8>, 20> results_array;
		double time_array = 0.0;

		// Finally we enter our main loop.
		while (1) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
			hub.run(1000 / 20);
			// After processing events, we call the print() member function we defined above to print out the values we've
			// obtained from any events that have occurred.

			//obtain data
			samples[cycles]= collector.print();
			cycles++;

			//once we reach 25 samples, calculate rms
			if (cycles >= samples.size()) {

				//various variable initialization
				std::ofstream emg_data_test;
				cycles = 0;
				std::array<double, 8> results = { 0, 0, 0, 0, 0, 0, 0, 0 };

				//perform sum
				for (int i = 0; i < samples.size(); i++) {
					for (int j = 0; j < samples[i].size(); j++) {
						double square = samples[i][j] * samples[i][j];
						results[j] += square;
					}

				}

				//perform mean and square root
				for (int i = 0; i < results.size(); i++) {
					results[i] = results[i] / samples.size();
					results[i] = sqrt(results[i]);
					
				}
				//populate results array
				for (int i = 0; i < results.size(); i++) {
					results_array[minute_counter][i] = results[i];

				}
				//results_array[minute_counter] = results;
				minute_counter++;
				
				//every 20 iterations, calculate slope of rms
				if (minute_counter >= 20) {
					//emg_data_test.open("emg_data_test.csv", std::ios_base::app);

					for (int i = 0; i < results.size(); i++) {
						double c0;
						double c1;
						double c00[results.size() * results_array.size()];
						double c01[results.size() * results_array.size()];
						double c11[results.size() * results_array.size()];
						double sumsq[results.size()];
						
						std::array<std::array<double, results_array.size()>, results.size()> results_for_reg;
						for (int j = 0; j < results.size(); j++) {
							results_for_reg[i][j] = results_array[j][i];
						}

						gsl_fit_linear(&time_array, sizeof(double), results_for_reg[i].data(), sizeof(double), results_for_reg[i].size(),
							&c0, &c1, c00, c01, c11,sumsq);
						int me = 0;
						//emg_data_test << c1 ;
						//emg_data_test << ",";
						if (i == 0) {
							most_recent = c1;
						}
	
						//emg_data_test << "\n";
						//emg_data_test << std::flush;
						//emg_data_test.close();

					}
					
					minute_counter = 0;
					time_array += .1;

				}

				if (previous_gradient != -1000000.0000000) {
					if ((most_recent < 0) && (most_recent < previous_gradient)) {
						std::cout << '\n' <<"fatige loop";
						for(int n=0; n<5;n++){
							myo->vibrate(myo->vibrationMedium);
							myo->vibrate(myo->vibrationShort);
							myo->vibrate(myo->vibrationShort);
						}
					}

				}
				
				previous_gradient = most_recent;
			}
		}

		// If a standard exception occurred, we print out its message and exit.
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}