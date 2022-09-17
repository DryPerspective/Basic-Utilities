#include "ConfigReader.h"

namespace IO {

	ConfigReader::ConfigReader(const std::string_view fileName, bool removeWS, bool setLowerCase) : m_removeWS{ removeWS }, m_lower{ setLowerCase } {
		addFileToMap(fileName);
	}

	void ConfigReader::close() {
		m_values.clear();
	}

	void ConfigReader::addFile(const std::string_view fileName, bool removeWS) {
		addFileToMap(fileName);
	}

	void ConfigReader::toLower(std::string& inString) const {
		for (char& letter : inString) {
			if (letter >= 65 && letter <= 90)letter += 32;
		}
	}


	void ConfigReader::addFileToMap(const std::string_view fileName) {
		std::ifstream file(std::string(fileName).c_str());
		std::string currentLine;
		while (std::getline(file, currentLine)) {
			//Remove WS if needed.
			if (m_removeWS) currentLine.erase(std::remove_if(currentLine.begin(), currentLine.end(), isspace), currentLine.end());

			//Ignore commented or empty lines
			if (currentLine[0] == '#' || currentLine.empty())continue;

			if (m_lower)toLower(currentLine);

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


			//And insert them
			m_values.insert(std::make_pair(lineBeforeEquals, lineAfterEquals));
		}
	}







	const char* ConfigReader::ConfigException::what() const noexcept {
		return m_message.c_str();
	}

}