
#include "MultiTimer.h"

namespace utility {

	//Type aliases to save some typing.
	using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;
	using duration_t = std::chrono::duration<double, std::ratio<1>>;

	//Constructor to store the initial time.
	MultiTimer::MultiTimer() {
		m_storedTimes.insert(std::make_pair(0, std::chrono::steady_clock::now()));
	}

	//Clear the map and reset the only entry to the current time.
	void MultiTimer::reset() {
		m_storedTimes.clear();
		m_storedTimes.insert(std::make_pair(0, std::chrono::steady_clock::now()));
	}

	//Add a particular time marker to the map.
	void MultiTimer::addTime(int inKey) {
		m_storedTimes.insert(std::make_pair(inKey, std::chrono::steady_clock::now()));
	}

	//Count how long has elapsed since the stored initial time.
	double MultiTimer::elapsed() const {
		return std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - m_storedTimes.at(0)).count();
	}

	//Count how long has elapsed since a given time.
	double MultiTimer::elapsed(int inKey) const {
		return std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - m_storedTimes.at(inKey)).count();
	}

	//Count how long elapsed between two stored times.
	double MultiTimer::elapsed(int inKey1, int inKey2) const {
		return std::chrono::duration_cast<duration_t>(m_storedTimes.at(inKey2) - m_storedTimes.at(inKey1)).count();
	}
}