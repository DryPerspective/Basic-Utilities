#include "ConfigReader.h"

namespace dp {

	ConfigReader::ConfigReader(std::string_view fileName, ConfigReader::flags inFlags) : m_flags{ inFlags } {
		addFileToMap(fileName);
	}

	void ConfigReader::close() {
		m_values.clear();
	}

	void ConfigReader::addFile(std::string_view fileName) {
		addFileToMap(fileName);
	}

	void ConfigReader::toLower(std::string& inString) {
		for (char& letter : inString) {
			if (letter >= 65 && letter <= 90)letter += 32;
		}
	}

	void toUpper(std::string& inString) {
		for (char& letter : inString) {
			if (letter >= 97 && letter <= 122)letter -= 32;
		}
	}

	std::string ConfigReader::trim(const std::string& str, std::string_view whitespace) {
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == std::string::npos) return "";

		const auto strEnd = str.find_last_not_of(whitespace);
		const auto strRange = strEnd - strBegin + 1;

		return str.substr(strBegin, strRange);
	}


	void ConfigReader::addFileToMap(std::string_view fileName) {
		std::ifstream file(std::string(fileName).c_str());
		if (file.fail()) {
			std::string errMsg{ "Error: File " };
			errMsg += fileName;
			errMsg += " could not be opened.";
			throw ConfigReader::ConfigException(errMsg);
		}
		std::string currentLine;
		while (std::getline(file, currentLine)) {
			//Remove WS if needed.
			if (m_flags & flags::removeAllWs) currentLine.erase(std::remove_if(currentLine.begin(), currentLine.end(), isspace), currentLine.end());

			//Ignore commented or empty lines
			if (currentLine[0] == '#' || currentLine.empty())continue;

			if (m_flags & caseInsensitive)toLower(currentLine);

			//Trim down comments at the end of lines
			auto comment{ currentLine.find('#') };
			if (comment != std::string::npos) {
				currentLine = currentLine.substr(0, comment);
			}

			//Split the line using = as a delimiter.
			auto splitPos{ currentLine.find('=') };
			if (splitPos == std::string::npos) throw ConfigReader::ConfigException("Error: Non-empty, non-commented line in config file does not contain an =");

			//Then use string_view to get two views of the line - one for each side of the delimiter
			std::string lineBeforeEquals{ currentLine.substr(0,splitPos) };
			std::string lineAfterEquals{ currentLine.substr(splitPos + 1, currentLine.length()) };
			if (lineBeforeEquals.empty() || lineAfterEquals.empty()) throw ConfigReader::ConfigException("Error: Empty line before or after = in config file");

			//If we're not otherwise keeping padding around the terms, we trim them down to just their text.
			if (!(m_flags & keepAllPadding)) {
				lineBeforeEquals = trim(lineBeforeEquals);
				lineAfterEquals = trim(lineAfterEquals);
			}

			//And insert them
			m_values.insert(std::make_pair(lineBeforeEquals, lineAfterEquals));
		}
	}







	const char* ConfigReader::ConfigException::what() const noexcept {
		return m_message.c_str();
	}

}