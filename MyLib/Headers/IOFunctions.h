#ifndef MYLIBIOFUNCTIONS
#define MYLIBIOFUNCTIONS
#pragma once

/*
* This file serves as a general purpose repository of some IO functions which don't belong in their own classes and structures. Admittedly not the most cohesive design.
* 
*/

#include <iostream>
#include <string>
#include <algorithm>

namespace IO {

	//A little crude, but this will read console input for a simple bool decision.
	bool getYesNo() noexcept;

	//A variation of the above function for custom input and decisions through the console.
	//E.g. the decision to hit or stand in blackjack is more easily recognised by "hit" or "stand" rather than a variation of "yes" and "no".
	bool getBinaryDecision(const std::string & trueValues, const std::string& falseValues) noexcept;


	// -------TEMPLATED FUNCTIONS------------

	//A basic boilerplate function to ignore a line in a general istream.
	template<typename T>
	void ignoreLine(std::basic_istream<T>& inStream) {
		inStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	//Boilerplate to get a value from the console with some input validation.
	template<typename T>
	void getFromConsole(T& inValue) noexcept {
		if constexpr (std::is_arithmetic_v<T>) {
			while (true) {
				std::cin >> inValue;	//Read our value
				ignoreLine(std::cin);	//And ignore anything left in the buffer.
				if (std::cin.fail()) {	//If extraction fails
					std::cin.clear();	//Reset our input stream flag
					std::cout << "Error: Please enter a valid " << typeid(T).name() << " value. \n";
				}
				else {
					return;
				}
			}
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			std::getline(std::cin, inValue);
		}
		else {
			std::clog << "Error: getFromConsole will not read " << typeid(T).name() << " types.\n";
		}
	}

	//Same as above except for cases of returning the input value rather than passing it in.
	template<typename T>
	T getFromConsole() noexcept {
		T input;
		return getFromConsole(input);
	}



}
#endif

