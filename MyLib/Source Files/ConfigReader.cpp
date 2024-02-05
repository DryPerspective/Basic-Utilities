#include "ConfigReader.h"

#include <cctype>
#include <cwctype>
#include <fstream>
#include <string_view>
#include <algorithm>

//These are internal to the workings of the class. The interface need not know about them.
namespace {

	//To prevent duplication
	constexpr auto first_non_space(std::string_view str) {
		//No constexpr find_if until C++20
#if __cplusplus >= 202002L
		//Grab the first non-ws character (don't forget cctype functions are a bit cursed
		return std::find_if(str.begin(), str.end(), [](char in) -> bool {return !std::isspace(static_cast<unsigned char>(in)); });
#else
		if (str.empty()) return str.end();
		for (std::string_view::const_iterator it = str.begin(); it != str.end(); ++it) {
			if(! static_cast<bool>(std::isspace(static_cast<unsigned char>(*it)))) return it;
		}

		return str.end();

#endif
	}

	constexpr auto last_non_space(std::string_view str) {
#if __cplusplus >= 202002L
		return std::find_if(str.rbegin(), str.rend(), [](char in) -> bool {return !std::isspace(static_cast<unsigned char>(in)); });
#else
		if (str.empty()) return str.rend();
		for (std::string_view::const_reverse_iterator it = str.rbegin(); it != str.rend(); ++it) {
			if (!static_cast<bool>(std::isspace(static_cast<unsigned char>(*it)))) return it;
		}

		return str.rend();

#endif
	}

	constexpr auto trim(std::string_view in) -> std::string_view {
		if (in.empty()) return in;

		auto first_pos{ first_non_space(in) };
		auto last_pos{ last_non_space(in) };

		if (first_pos == in.end() || last_pos == in.rend()) return in;
		
		return std::string_view{ &*first_pos, static_cast<std::string_view::size_type>(std::distance(first_pos, last_pos.base())) };
	}

	auto parse_file_line(std::string_view line) -> std::pair<std::string_view, std::string_view> {
		//Remove any line comments.
		auto comment_start{ line.find('#') };
		if (comment_start != std::string_view::npos) line.remove_suffix(line.size() - line.find('#'));

		//Find the =
		auto equals_pos{ std::find(line.begin(), line.end(), '=') };
		if (equals_pos == line.end()) throw dp::ConfigReader::ConfigException("Line " + std::string{ line } + " is not empty, commented, and does not contain a =");



		//I'll explain the cursed part of this
		/*
		*  In MSVC, string_view iterators aren't implemented as CharT* so in C++17 we need to do this silly dance of &*
		*  We know at this point that the string contains at least one non-ws term (the =) so it's safe to do
		*  The string_view iterator constructor is a C++20 feature so we need a call to std::distance and an ugly cast
		*/
		auto start{ first_non_space(line) };
		//We have enough information to grab what we need now
		std::string_view key_term{ &*start , static_cast<std::string_view::size_type>(std::distance(start, equals_pos)) };
		++equals_pos;
		std::string_view val_term{ &*equals_pos, static_cast<std::string_view::size_type>(std::distance(equals_pos,line.end())) };

		return std::make_pair(trim(key_term), trim(val_term));
	}

	//We exploit the old case-insensitive char_traits trick for good comparison
	template<typename T>
	constexpr inline bool is_char_or_wchar = std::is_same_v<T, char> || std::is_same_v<T, wchar_t>;

	template<typename T, std::enable_if_t<is_char_or_wchar<T>, bool> = true>
	class ci_traits : public std::char_traits<T>{
	
		static T conv(T in) noexcept {
			if constexpr (std::is_same_v<T, char>) {
				return static_cast<char>(std::toupper(static_cast<unsigned char>(in)));
			}
			else {
				return static_cast<wchar_t>(std::towupper(in));
			}
		}

	public:

		static bool eq(T lhs, T rhs) noexcept {
			return conv(lhs) == conv(rhs);
		}

		static bool lt(T lhs, T rhs) noexcept {
			return conv(lhs) < conv(rhs);
		}

		static int compare(const T* lhs, const T* rhs, std::size_t n){
			while (n-- != 0)
			{
				if (conv(*lhs) < conv(*rhs))
					return -1;
				if (conv(*lhs) > conv(*rhs))
					return 1;
				++lhs;
				++rhs;
			}
			return 0;		
		}

		static const T* find(const T* ptr, std::size_t n, T in_val) {
			const auto val{ conv(in_val) };
			while (n-- != 0) {
				if (conv(*ptr) == val) return ptr;
				++ptr;
			}
			return nullptr;
		}	
	};

	using ci_view = std::basic_string_view<char, ci_traits<char>>;

	template<typename dest_traits, typename CharT, typename src_traits>
	std::basic_string_view<CharT, dest_traits> traits_cast(std::basic_string_view<CharT, src_traits> in) noexcept {
		return { in.data(), in.size() };
	}


}



namespace dp {


	void ConfigReader::addFile(std::filesystem::path in_file) {
		if (!std::filesystem::exists(in_file)) throw ConfigException("Requested file does not exist");

		std::ifstream file_input{ in_file };
		std::string current_line{};
		while (std::getline(file_input, current_line)) {			
			auto first_char = first_non_space(current_line);
			//Skip on empty lines and comment lines (start with #)
			//Unfortunate cast needed for the comparison.
			if (first_char == std::end(std::string_view{ current_line }) || *first_char == '#') continue;

			//Then insert a key-value pair separated by equals.
			file_contents.insert(parse_file_line(current_line));
		}

	}

	ConfigReader::ConfigReader(std::filesystem::path in_path) {
		addFile(in_path);
	}

	auto  ConfigReader::GetValue(std::string_view key) const -> std::optional<std::string_view> {

		auto val_pos = std::find_if(file_contents.begin(), file_contents.end(), [key = trim(key)](const map_type::value_type& in) {
			auto ci_val = traits_cast<ci_traits<char>>(std::string_view(in.first));
			return ci_val == traits_cast<ci_traits<char>>(key);
		});

		if (val_pos == file_contents.end()) return std::nullopt;
		
		return val_pos->second;
	}

	auto ConfigReader::clear() -> void {
		file_contents.clear();
	}

	auto ConfigReader::close() -> void {
		clear();
	}

}