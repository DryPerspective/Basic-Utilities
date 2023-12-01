#ifndef MYLIBIOFUNCTIONS
#define MYLIBIOFUNCTIONS


/*
* This file serves as a general purpose repository of some IO functions which don't belong in their own classes and structures. Admittedly not the most cohesive design.
* 
*/

#include <iostream>
#include <string_view>
#include <charconv>

#include "Traits.h"

namespace dp {

	//A little crude, but this will read console input for a simple bool decision.
	bool getYesNo();

	//A variation of the above function for custom input and decisions through the console.
	//E.g. the decision to hit or stand in blackjack is more easily recognised by "hit" or "stand" rather than a variation of "yes" and "no".
	bool getBinaryDecision(std::string_view trueValues, std::string_view falseValues);


	// -------TEMPLATED FUNCTIONS------------


	//Boilerplate to get a value from the console with some input validation.
	template<typename T, std::enable_if_t<dp::isExtractible<T>::value, bool> = true>
	void getFromConsole(T& inValue) {

		while (true) {
			std::cin >> inValue;

			if (std::cin.fail()) {	//If extraction fails
				std::cin.clear();	//Reset our input stream flag
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
				//Would be nice if we had a better way which was portable. Alas, no __PRETTY_FUNCTION__ hacks or reflection.
				std::cout << "Error: Please enter a valid " << typeid(T).name() << " value. \n";
			}
			else {
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//Ignore anything left in the buffer
				return;
			}
		}
		
	}
	//Same as above except for cases of returning the input value rather than passing it in.
	template<typename T>
	T getFromConsole() {
		T input;
		return getFromConsole(input); 
	}


	//Boilerplate for a from_chars read. As its parameters vary between integral and floating point types, we use a bit of SFINAE to isolate those types
	//This allows us to vary our function parameters depenging on the type fed in, as needed.
	//Note that these are not intended as a one-size-fits-all substitution for intelligent and considered use of from_chars, but provide a simple way to get the data
	//in situations where the benefits being bypassed aren't as relevant.
	template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
	T getFromChars(std::string_view inputString, int base = 10) {
		T output{};

		auto [ptr, errc] {std::from_chars(inputString.data(), inputString.data() + inputString.length(), output, base)};

		if (ptr == inputString.data() + inputString.length()) return output;
		else if (ptr == inputString.data()) {
			if (errc == std::errc::invalid_argument) throw std::invalid_argument("Bad from_chars argument");
			else if (errc == std::errc::result_out_of_range) throw std::out_of_range("From_chars argument out of range");
		}
		else {
			//throw std::invalid_argument("Partial from_chars match");
		}
		return output;
	}

	//Noexcept variant, mirroring some std constructs.
	//Does not account for the edge case of a partial match, admittedly, but in most cases the user should be the one to avoid that.
	template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
	T getFromChars(std::string_view inputString, std::errc& errc, int base = 10) noexcept {
		T output{};

		auto result{ std::from_chars(inputString.data(), inputString.data() + inputString.length(), output, base)};
		errc = result.errc;
		return output;
	}

	//Non-throwing overload which returns a bool to determine success or failure
	template<typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
	bool getFromChars(std::string_view inputString, T& value, int base = 10) noexcept {
		auto [ptr, errc] {std::from_chars(inputString.data(), inputString.data() + inputString.length(), value, base)};
		if (ptr == inputString.data() + inputString.length()) return true;
		else return false;
	}

	//And on to floating point types.
	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
	T getFromChars(std::string_view inputString, std::chars_format fmt = std::chars_format::general) {
		T output{};
		auto [ptr, errc] {std::from_chars(inputString.data(), inputString.data() + inputString.length(), output, fmt)};
		if (ptr == inputString.data() + inputString.length()) return output;
		else if (ptr = inputString.data()) {
			if (errc == std::errc::invalid_argument)throw std::invalid_argument("Bad from_chars argument");
			else if (errc == std::errc::result_out_of_range) throw std::out_of_range("From_chars argument out of range");
		}
		else {
			//throw std::invalid_argument("Partial from_chars match");
		}

		return output;
	}

	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
	T getFromChars(std::string_view inputString, std::errc& errc, std::chars_format fmt = std::chars_format::general) noexcept {
		T output{};
		auto result{ std::from_chars(inputString.data(), inputString.data() + inputString.length(), output, fmt)};
		errc = result.errc;
		return output;
	}

	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
	bool getFromChars(std::string_view inputString, T& value, std::chars_format fmt = std::chars_format::general) noexcept {
		auto [ptr, errc] {std::from_chars(inputString.data(), inputString.data() + inputString.length(), value, fmt)};
		if (ptr == inputString.data() + inputString.length()) return true;
		else return false;
	}


}
#endif

