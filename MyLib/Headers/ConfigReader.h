#ifndef CONFIGREADER
#define CONFIGREADER


/*
 ConfigReader - A class intended to automate the reading of configuration files and binding of variables to those values.
 In short, many of my projects need to read a file of the format:
 Variable1=[Value]
 Variable2=[Value]
 A few notes on file layout - whitespace and empty lines are ignored by the program, and lines in the config file can be "commented" by starting them with #
 In order to configure themselves and get the correct values into the program. This class is intended to take a file of that layout and automate the reading of it.

 Values can be retrieved via the readValue member function. By default this will retrieve a string corresponding to the key in the file, however templated versions
 are included which perform conversion to an appropriate type

*/

#include <type_traits>
#include <filesystem>
#include <unordered_map>
#include <string>
#include <optional>
#include <charconv>

#include "Traits.h"

namespace dp {

	//String_view starts_with functionality, here until I find a better place to put it
	inline constexpr auto StartsWith(std::string_view src, std::string_view prefix) -> bool {
		return std::string_view{ src.data(), std::min(src.size(), prefix.size()) } == prefix;
	}


	class ConfigReader {

		using map_type = std::unordered_map<std::string, std::string>;
		map_type file_contents;

		//Value lookup from a key
		auto GetValue(std::string_view key) const -> std::optional<std::string_view> ;

		template<typename Dest>
		static constexpr auto FromString(std::string_view in) -> Dest {
			//Easy conversion first
			if constexpr (std::is_convertible_v <std::string_view, Dest>) {
				return static_cast<Dest>(in);
			}
			//Integral types via charconv
			else if constexpr (std::is_integral_v<Dest>) {
				auto base{ 10 };
				if (StartsWith(in, "0x")) {
					base = 16;
					in.remove_prefix(2);
				}
				Dest ret{};
				auto result{ std::from_chars(in.data(), in.data() + in.size(), ret, base) };
				if (result.ec == std::errc{}) {
					return ret;
				}
				else if(result.ec == std::errc::invalid_argument){
					throw ConfigException{ "Could not convert " + std::string{in} + ": Invalid argument" };
				}
				else {
					throw ConfigException{ "Coult not convert " + std::string{in} + ": Out of range" };
				}
			}
			//Floating point types via charconv
			else if constexpr (std::is_floating_point_v<Dest>) {
				std::chars_format fmt{ std::chars_format::general };
				if (StartsWith(in, "0x")) {
					fmt = std::chars_format::hex;
					in.remove_prefix(2);
				}
				Dest ret{};
				auto result{ std::from_chars(in.data(), in.data() + in.size(), ret, fmt)};
				if (result.ec == std::errc{}) {
					return ret;
				}
				else if (result.ec == std::errc::invalid_argument) {
					throw ConfigException{ "Could not convert " + std::string{in} + ": Invalid argument" };
				}
				else {
					throw ConfigException{ "Coult not convert " + std::string{in} + ": Out of range" };
				}
			}
			else {
				//I hate this ugly hack.
				static_assert(dependentFalse<Dest>, "FromString called with invalid type");
			}
		}


	public:

		ConfigReader(std::filesystem::path in_path);

		auto addFile(std::filesystem::path in_path) -> void;

		auto clear() -> void;

		template<typename T>
		constexpr auto readValue(std::string_view in) const -> T {
			auto val = GetValue(in);
			if (!val.has_value()) throw ConfigException("Value " + std::string{ in } + " not found in map");
			return FromString<T>(*val);
		}

		template<typename T>
		constexpr auto readValue(std::string_view in, const T& default_value) const noexcept -> T {
			auto val = GetValue(in);
			if (!val.has_value()) return default_value;
			try {
				return FromString<T>(*val);
			}
			catch (...) {
				return default_value;
			}
		}

		//Non-template approach just fetches the key
		constexpr auto readValue(std::string_view in) const -> std::string_view {
			return readValue<std::string_view>(in);
		}

		struct ConfigException : public std::runtime_error {
			using Base = std::runtime_error;

			using Base::Base;
		};



		//A few deprecated calls for backwards compatibility
		//To be removed in a future update
		enum flags : unsigned char { //This was always an overdesign
			noFlagsActive = 0,
			removeAllWs = 0b00000001,		
			keepAllPadding = 0b00000010,		
			caseInsensitive = 0b00000100,		
		};

		[[deprecated("ConfigReader flags have been removed")]]
		ConfigReader(std::filesystem::path in_path, flags) : ConfigReader(in_path) {}

		[[deprecated("This function is deprecated. Use clear() instead")]]
		auto close() -> void;

		[[deprecated("This function is deprecated. Use readValue instead")]]
		constexpr auto getValue(std::string_view in) -> std::string_view {
			return readValue(in);
		}


	};



}

#endif
