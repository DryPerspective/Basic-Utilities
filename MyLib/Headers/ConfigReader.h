#ifndef CONFIGREADER
#define CONFIGREADER


/*
 ConfigReader - A class intended to automate the reading of configuration files and binding of variables to those values.
 In short, many of my projects need to read a file of the format:
 Variable1=[Value]
 Variable2=[Value]
 A few notes on file layout - by default whitespace and empty lines are ignored by the program, and lines in the config file can be "commented" by starting them with #
 In order to configure themselves and get the correct values into the program. This class is intended to take a file of that layout and automate the reading of it.

 This class does appear from the outset to be rather memory-heavy. And to an extent this is true - it internally stores a map of strings of all values in the input
 file at once. However the intention is that the entire ConfigReader class can be created, its values read, and then it (implicitly) destructed in a single try block
 near the beginning of the program. It also includes a close() member function to empty the map if direct control over that is needed.

 The main readValue function supports the following types:
 * Fundamental integer and floating point types, written in base-10, scientific, or hex notation, though hex must be prefixed with "0x" in the file
 * Char
 * Bool
 * std::string
 In addition it also supports genertic types T which meet *any* of the following criteria
 * Must be assignable from std::string, i.e. std::is_assignable_v<T, std::string> must be true.
 * Must be constructable from a std::string_view and no other parameters, and must be either move- or copy-assignable, with move-assignment taking priority.
 * Must be constructable from a std::string and no other parameters, and must be either move- or copy-assignable, with move-assignment taking priority.
 * Must be extractable from an std::istream using operator>>, such that an extraction with types std::string >> T will evaluate correctly.
*/


#include<fstream>
#include<string>
#include<string_view>
#include<unordered_map>
#include<exception>
#include<charconv>
#include<regex>
#include<type_traits>
#include<stdexcept>
#include<sstream>
#include<filesystem>

#include "Traits.h"



	namespace dp {

		class ConfigReader
		{

		public:
			//Configuration flags. Declared here so we can use them as private member data.
			enum flags : unsigned char {
				noFlagsActive = 0,
				removeAllWs = 0b00000001,		//Remove white space from inside the text in the config file. e.g. Message = Hello World -> {Message,HelloWorld}
				keepAllPadding = 0b00000010,		//Keep any white space "padding" left between the equals and the value and after the value in the config file, e.g. Message = Hello World -> {Message , Hello World}
				caseInsensitive = 0b00000100,		//Set all text read in to lower case for both storage and retrieval. This means input is not case sensitive.
			};


		private:



			//Encapsulation as we want a complete wrapper around the data container and want to prevent external meddling with it.
			std::unordered_map<std::string, std::string>	m_values;
			ConfigReader::flags								m_flags;

			//Adds all data from a file to the m_values map. Separated from constructor for use with addFile function.
			void addFileToMap(std::string_view fileName);


			//Simple function to trim any leading or trailing whitespace from a string,
			static std::string trim(const std::string& str, std::string_view whitespace = " \f\n\r\t\v");

			//Sets all text to lower case, used for the caseInsensitive flag
			static void toLower(std::string& inString);

			//This function is called to read a string representing a number into a numerical type.
			template<typename T>
			constexpr T readNumerical(std::string_view valueInFile) const {

				//Then use from_chars to read it.
				T output{};
				std::from_chars_result result;


				//Integer and floating point values use from_chars differently, so we separate here
				if constexpr (std::is_floating_point<T>::value) {

					//First deduce the format of our final result.
					auto format{ std::chars_format::general };
					//Regex for scientific format numbers. It can contain any number of numerical digits, optionally a decimal point, either e or E for exponent, optionally a -, then any number of digits.
					std::regex scientific{ R"(\-?\d+\.?\d*[eE]\-?\d+)" };
					//Ditto for hex. Assumes that hex numbers are prefixed with "0x" in the file
					std::regex hex{ R"(0x\-?[\da-fA-F]+\.?[\da-fA-F]*)" };
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
				//This will never trigger but we need to return something from all paths.
				return output;
			}



		public:

			//We don't want default or copy-instantiation so we disable those constructors.
			ConfigReader() = delete;
			ConfigReader(const ConfigReader&) = delete;

			//There's no particular reason that move construction should be forbidden, though its expected use will be a bit niche.
			ConfigReader(ConfigReader&&) noexcept = default;

			//We only want construction when we have a specific file to open and read.
			ConfigReader(std::string_view fileName, ConfigReader::flags inFlags = flags::noFlagsActive);
			ConfigReader(std::filesystem::path file, ConfigReader::flags inFlags = flags::noFlagsActive);

			 ~ConfigReader() noexcept = default;

			//Free up memory from the map for cases where we are done with the ConfigReader but it remains in scope
			void close();

			//Read an additional file and insert its data into the map.
			void addFile(std::string_view fileName);
			void addFile(std::filesystem::path fileName);


			//This is the core function. It it the one-size-fits-all function to match a string which was in the file to some variable within the program.
			//It has support for numeric types, strings, and PhysicsVector objects. As we're ostensibly reading from strings, exotic custom types can be read
			//by returning the string and processing it in the main project file.
			template <typename T>
			void readValue(std::string varNameInFile, T& inVariable) const {
				if (m_flags & flags::caseInsensitive) {
					toLower(varNameInFile);
				}
				std::string valueInMap;
				try {
					valueInMap = m_values.at(varNameInFile);
				}
				catch (const std::out_of_range&) {
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
				//Bool.
				else if constexpr (std::is_same_v<T, bool>) {
					switch (valueInMap[0]) {
					case 't':	//true
					case 'T':
					case 'y':	//yes
					case 'Y':
					case '1':	//generic 1
						inVariable = true;
						break;
					case 'f':	//false
					case 'F':
					case 'n':	//no
					case 'N':
					case '0':	//generic 0
						inVariable = false;
						break;
					default:
						throw ConfigReader::ConfigException("Error: Attempting to read bool but variable in file is set to incorrect value.");
					}
				}
				//Numerical types
				else if constexpr (std::is_arithmetic_v<T>) {
					inVariable = readNumerical<T>(valueInMap);
				}
				//GENERIC TYPES
				//Assigned directly by a std::string
				else if constexpr (std::is_assignable_v<T, std::string>) {
					inVariable = valueInMap;
				}
				//Constructble from a string_view along and can be move assigned
				else if constexpr (std::is_constructible_v<T, std::string_view> && std::is_move_assignable_v<T>) {
					inVariable = std::move(T{ valueInMap });
				}
				else if constexpr (std::is_constructible_v<T, std::string_view> && std::is_copy_assignable_v<T>) {
					//Note we need to make a temporary as we need an lvalue, since this branch can trigger if the class
					//explicitly cannot move-assign.
					T temp{ valueInMap };
					inVariable = temp;
				}
				//Constructible from a string alone and can be move assigned
				else if constexpr (std::is_constructible_v<T, std::string> && std::is_move_assignable_v<T>) {
					inVariable = std::move(T{ valueInMap });
				}
				//Constructable from a string alone and can be copy assigned.
				else if constexpr (std::is_constructible_v<T, std::string> && std::is_copy_assignable_v<T>) {
					//Again we need an lvalue here.
					T temp{ valueInMap };
					inVariable = temp;
				}
				else if constexpr (dp::isExtractible_v<T>) {
					std::stringstream sstr;
					sstr << valueInMap;
					sstr >> inVariable;
				}
				/*
				//C-srings
				else if constexpr (std::is_array_v<T> && std::is_same_v<char&, decltype(*inVariable)>) {
					if (valueInMap.length() > strlen(inVariable)) throw ConfigReader::ConfigException("Error: Attempting to match variable in file to a C-string which is too short to contain it");
					else strcpy_s(inVariable, valueInMap.c_str());
				}
				*/
				else static_assert(dp::dependentFalse<T>, "ConfigReader::readValue called with unsupported type");

			}

			//A more conventional method without an in-out parameter. Logically this can only apply to default constructble and copyable types, so we filter for those
			//(and hope that RVO saves us a copy anyway)
			template<typename T, std::enable_if_t<std::is_default_constructible_v<T> && std::is_copy_constructible_v<T>,bool> = true>
			T readValue(std::string_view VarNameInFile) {
				T result{};
				readValue(VarNameInFile, result);
				return result;
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
