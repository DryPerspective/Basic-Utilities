#include "IOFunctions.h"

namespace IO {

	bool getYesNo() noexcept {
		char input;
		while (true) {
			std::cin >> input;
			switch (input) {
			case 'y':
			case 'Y':
			case '1':
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	//Clear any extraneous input
				return true;
			case 'n':
			case 'N':
			case '0':
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	
				return false;
			default:
				std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');	
				std::cout << "Error: Please enter a valid value.\n";
			}


		}
	}


}
