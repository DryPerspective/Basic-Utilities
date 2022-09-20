#ifndef MYLIBIOFUNCTIONS
#define MYLIBIOFUNCTIONS
#pragma once

/*
* This file serves as a general purpose repository of some IO functions which don't belong in their own classes and structures. Admittedly not the most cohesive design.
* 
*/

#include<iostream>
#include<string>

namespace IO {

	//A little crude, but this will read console input for a simple bool decision.
	bool getYesNo() noexcept;


	// -------TEMPLATED FUNCTIONS------------

	//Boilerplate to get a value from the console with some input validation.
	template<typename T>
	void getFromConsole(T& inValue) noexcept {
		if constexpr (std::is_same_v<T, bool>) {
			T = getYesNo();
		}
		else if constexpr (std::is_arithmetic_v<T>) {
			while (true) {
				std::cin >> inValue;
				if (std::cin.fail()) {	//If extraction fails
					std::cin.clear();	//Reset our input stream flag
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//And ignore anything left in the buffer.
					std::cout << "Error: Please enter a valid " << typeid(T).name() << " value. \n";
				}
				else {
					std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//Clear any extraneous input
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

