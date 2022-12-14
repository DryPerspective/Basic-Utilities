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
#include <string_view>
#include <charconv>

namespace IO {

	//A little crude, but this will read console input for a simple bool decision.
	bool getYesNo() noexcept;

	//A variation of the above function for custom input and decisions through the console.
	//E.g. the decision to hit or stand in blackjack is more easily recognised by "hit" or "stand" rather than a variation of "yes" and "no".
	bool getBinaryDecision(const std::string& trueValues, const std::string& falseValues) noexcept;


	// -------TEMPLATED FUNCTIONS------------

	//Quick trait for a value which can be extracted from an istream.
	namespace {
		template<typename, typename = std::void_t<>>
		struct isExtractible : std::false_type {};

		template<typename T>
		struct isExtractible<T, std::void_t<decltype(std::declval<std::istream&>() >> std::declval<T&>())>> : std::true_type {};
	}

	//A basic boilerplate function to ignore a line in a general istream.
	template<typename T>
	void ignoreLine(std::basic_istream<T>& inStream = std::cin) {
		inStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}

	//Boilerplate to get a value from the console with some input validation.
	template<typename T, std::enable_if_t<isExtractible<T>::value, bool> = true>
	void getFromConsole(T& inValue) noexcept {
		if constexpr (std::is_same_v<T, bool>) {
			inValue = getYesNo();
		}
		else {
			while (true) {
				std::cin >> inValue;

				if (std::cin.fail()) {	//If extraction fails
					std::cin.clear();	//Reset our input stream flag
					ignoreLine(std::cin);	//Ignore anything left in the buffer
					std::cout << "Error: Please enter a valid " << typeid(T).name() << " value. \n";
				}
				else {
					ignoreLine(std::cin);	//Ignore anything left in the buffer
					return;
				}
			}
		}
	}

	//Same as above except for cases of returning the input value rather than passing it in.
	template<typename T>
	T getFromConsole() noexcept {
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

		auto result{ std::from_chars(inputString.data, inputString.data() + inputString.length(), output, base) };
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
		auto result{ std::from_chars(inputString.data, inputString.data() + inputString.length(), output, fmt) };
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

