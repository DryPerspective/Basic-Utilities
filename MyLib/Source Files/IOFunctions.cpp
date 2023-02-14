#include "IOFunctions.h"

namespace IO {

	bool getYesNo() {
		char input;
		while (true) {
			getFromConsole(input);
			switch (input) {
			case 'y':	//Yes
			case 'Y':
			case 't':	//True
			case 'T':
			case '1':
				return true;
			case 'n':	//No
			case 'N':
			case 'f':	//False
			case 'F':
			case '0':
				return false;
			default: 
				std::cout << "Error: Please enter a valid value.\n";
			}


		}
	}

	bool getBinaryDecision(const std::string& trueValues, const std::string& falseValues)
	{
		char input;
		while (true) {
			getFromConsole(input);
			if (std::any_of(trueValues.begin(), trueValues.end(), [input](const char& x) {return x == input; })) return true;
			if (std::any_of(falseValues.begin(), falseValues.end(), [input](const char& x) {return x == input; })) return false;
			else {
				std::cout << "Error: Please enter a valid option.\n";
			}
		}
	}


}
