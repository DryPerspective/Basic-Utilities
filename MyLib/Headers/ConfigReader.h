#ifndef CONFIGREADER
#define CONFIGREADER

#pragma once

/*
 ConfigReader - A class intended to automate the reading of configuration files and binding of variables to those values. 
 In short, many of my projects need to read a file of the format:
 Variable1=[Value]
 Variable2=[Value]
 In order to configure themselves and get the correct values into the program. This class is intended to take a file of that layout and automate the reading of it.
*/

#include<iostream>
#include<fstream>
#include<string>
#include<string_view>
#include<unordered_map>
#include<exception>
#include<charconv>
#include<regex>
#include<type_traits>
#include<stdexcept>

#include "PhysicsVector.h"

namespace IO {

	class ConfigReader
	{
	private:

		//Encapsulation as we want a complete wrapper around the data container and want to prevent external meddling with it.
		std::unordered_map<std::string, std::string>	m_values;
		bool											m_lower;		//Whether to process all string text to lower case to prevent some potential misspellings.
		bool											m_removeWS; 	//Whether to remove all whitespace from processed text in the file.

		//Adds all data from a file to the m_values map. Separated from constructor for use with addFile function.
		void addFileToMap(const std::string_view fileName);

		//To save time and remove case-sensitivity, this function will set all letters in the string to lower case.
		void toLower(std::string& inString) const;

		//This function is called to read a string representing a number into a numerical type.
		template<typename T>
		T readNumerical(std::string_view valueInFile) const {

			//Then use from_chars to read it.
			T output;
			std::from_chars_result result;


			//Integer and floating point values use from_chars differently, so we separate here
			if constexpr (std::is_floating_point<T>::value) {

				//First deduce the format of our final result.
				auto format{ std::chars_format::general };
				//Regex for scientific format numbers. It can contain any number of numerical digits, optionally a decimal point, either e or E for exponent, optionally a -, then any number of digits.
				std::regex scientific{ "[\\-]?[0-9]+[\\.]?[0-9]*[eE][\\-]?[0-9]+" };
				//Ditto for hex. Assumes that hex numbers are prefixed with "0x" in the file
				std::regex hex{ "0x[\\-]?[0-9a-fA-F]+[\\.]?[0-9a-fA-F]*" };
				if (std::regex_match(std::string(valueInFile), scientific))format = std::chars_format::scientific;
				else if (std::regex_match(std::string(valueInFile), hex)) {
					format = std::chars_format::hex;
					//We need to remove the 0x prefix as from_chars can't parse it.
					valueInFile.remove_prefix(2);
				}
				//std::string readString{ valueInFile };
				result = std::from_chars(valueInFile.data(), valueInFile.data() + valueInFile.length(), output, format);
			}
			else {
				result = std::from_chars(valueInFile.data(), valueInFile.data() + valueInFile.length(), output);
			}

			//Now onto error handling and ensuring that we got the right result.
			//As the success largely depends on the user entering the correct data in the input file, we throw exceptions when they do not.
			if (result.ptr == valueInFile.data() + valueInFile.length()) return output;
			else if (result.ec == std::errc::invalid_argument) {
				std::string errMsg{ "Error: Value " };
				errMsg += valueInFile;
				errMsg += " in file is of an invalid format.";
				throw ConfigReader::ConfigException(errMsg);
			}
			else if (result.ec == std::errc::result_out_of_range) {
				std::string errMsg{ "Error: Value " };
				errMsg += valueInFile;
				errMsg += " is larger than the type it is being read as.";
				throw ConfigReader::ConfigException(errMsg);
			}
		}

		//This function will read a PhysicsVector object from the output file, as this has proven to be a common need for some projects.
		template<std::size_t dim>
		void readVector(std::string_view valueInFile, Physics::PhysicsVector<dim>& inVector) const {

			//First, validate that the string is of the correct format (X,Y,Z)
			std::regex vectorReg{ "[\\{\\[\\(<]?([0-9]*[\\.]?[0-9]*[\\,]?){0,}[0-9]+[\\.]?[0-9]*[\\}\\]\\)>]?" };	
			//Optionally one of [{(<, then any amount of (0-9, optionally with a . and another [0-9] then ,), then another potentially decimal number and optionally a closing bracket
			if (!std::regex_match(std::string(valueInFile), vectorReg)) {
				std::string errMsg{ "Error: Vector " };
				errMsg += valueInFile;
				errMsg += " does not match the correct vector format.";
				throw ConfigReader::ConfigException(errMsg);
			}
			//We delimit around the comma to reach our individual numbers.
			//As we cannot easily insert our dim variable into the regex, the simplesy way to check we have the right number of dimensions is counting the number of commas
			auto numberOfCommas{ std::count(valueInFile.begin(), valueInFile.end(),',') };
			if (numberOfCommas != dim - 1) {
				std::string errMsg{ "Error: Vector " };
				errMsg += valueInFile;
				errMsg += " has an incorrect number of dimensions.";
				throw ConfigReader::ConfigException(errMsg);
			}

			//If the vector is surrounded by brackets, we need to trim them off. We account for all common bracket styles.
			std::string brackets{ "{}[]()<>" };
			if (std::any_of(brackets.begin(), brackets.end(), [valueInFile](const char& x) {return x == valueInFile[0]; })) valueInFile.remove_prefix(1);
			if (std::any_of(brackets.begin(), brackets.end(), [valueInFile](const char& x) {return x == valueInFile[valueInFile.length() - 1]; })) valueInFile.remove_suffix(1);

			//If we get this far, we have a good degree of confidence that our vector is of the correct format and that external brackets have been trimmed.
			//All that remains is to separate out the numbers, read them, and write them to a PhysicsVector object. 
			//This is trivial for 1D vectors, and in this case our entire valueInFile string should just be the number we want.
			if constexpr (dim == 1) {
				inVector.setAt(0, readNumerical(valueInFile));
			}
			//Otherwise we just read every number up to each comma, and set the vector accordingly
			else {
				for (std::size_t i = 0; i < dim; ++i) {
					auto firstComma{ valueInFile.find_first_of(',') };
					std::string_view firstTerm{ valueInFile.substr(0,firstComma) };
					valueInFile.remove_prefix(firstComma + 1);
					inVector.setAt(i, readNumerical<double>(firstTerm));
				}
			}
		}


	public:

		//We don't want default or copy-instantiation so we disable those constructors.
		ConfigReader() = delete;
		ConfigReader(const ConfigReader&) = delete;

		//We only want construction when we have a specific file to open and read.
		ConfigReader(const std::string_view fileName, bool removeWS = true, bool setLowerCase = false);

		//We want out destructor virtual in case of inheritance, but we will also handle closing the file here.
		virtual ~ConfigReader() = default;

		//Free up memory from the map for cases where we are done with the ConfigReader but it remains in scope
		void close();


		//Read an additional file and insert its data into the map.
		void addFile(const std::string_view fileName, bool removeWS = true);		


		//This is the core function. It it the one-size-fits-all function to match a string which was in the file to some variable within the program.
		//It has support for numeric types, strings, and PhysicsVector objects. As we're ostensibly reading from strings, exotic custom types can be read
		//by returning the string and processing it in the main project file.
		template <typename T>
		void readValue(std::string varNameInFile, T& inVariable) const {
			if (m_lower)toLower(varNameInFile);
			std::string valueInMap;
			try {
				valueInMap = m_values.at(varNameInFile);
			}
			catch (std::out_of_range e) {
				std::string errMsg{ "Error: Attempting to match value " };
				errMsg += varNameInFile;
				errMsg += " however this does not appear in the config file.";
				throw ConfigReader::ConfigException(errMsg);
			}

			//Different types handle differently. Char comes first as it would also be caught in is_arithmetic
			if constexpr (std::is_same<T, char>::value) {
				inVariable = valueInMap[0];
			}
			//std::string
			else if constexpr (std::is_same_v<T, std::string>) {
				inVariable = valueInMap;
			}
			//Numerical types
			else if constexpr (std::is_arithmetic_v<T>) {
				inVariable = readNumerical<T>(valueInMap);
			}
			else if constexpr (Physics::is_PhysicsVector<T>::value) {
				readVector(valueInMap, inVariable);
			}
			//C-srings
			else if constexpr (std::is_array_v<T> && std::is_same_v<char&, decltype(*inVariable)>) {
				if (valueInMap.length() > strlen(inVariable)) throw ConfigReader::ConfigException("Error: Attempting to match variable in file to a C-string which is too short to contain it");
				else strcpy_s(inVariable, valueInMap.c_str());
			}


			else throw ConfigReader::ConfigException("Error - readValue only supports numerical types, C-strings, and std::string");

		}




		//A custom exception to handle errors specific to ConfigReader operation.
		class ConfigException : public std::exception {
		private:
			std::string m_message;

		public:
			ConfigException(std::string_view message) : m_message{ message } {};

			virtual ~ConfigException() = default;

			const char* what() const noexcept override;
		};



	};
}

#endif
