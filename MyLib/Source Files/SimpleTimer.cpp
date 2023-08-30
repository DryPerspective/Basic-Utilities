#include "SimpleTimer.h"

namespace dp {

	//Reset the clock
	void SimpleTimer::reset() {
		m_trackedTime = std::chrono::steady_clock::now();
	}

	//Return number of seconds elapsed.
	double SimpleTimer::elapsed() {
		return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(std::chrono::steady_clock::now() - m_trackedTime).count();
	}

}