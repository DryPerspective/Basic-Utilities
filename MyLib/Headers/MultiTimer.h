#ifndef MYLIBMULTITIMER
#define MYLIBMULTITIMER
#pragma once

#include <chrono>
#include <string>
#include <map>
#include <utility>	//For std::make_pair()

namespace utility {

	/*
	* The MultiTimer object uses the chrono header to track multiple different times. Each time is stored internally in a map and can be called up as needed.
	* The keys on this map are given by strings, to allow a recognisable connection between the time you want and the point stored internally.
	*/
	class MultiTimer
	{
	private:
		using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;		

		std::map<int, timepoint_t> m_storedTimes;

	public:
		//Constructor to set up the initial time.
		MultiTimer();

		//Clear the map and reset the initial time to now.
		void reset();

		//Add a new time to the clock
		void addTime(int inKey);

		//Determine how long has elapsed since the initial time
		double elapsed() const;

		//Determine how long has elapsed since a given point.
		double elapsed(int inKey) const;

		//And determine how long elapsed between two stored times.
		double elapsed(int inKey1, int inKey2) const;

	};
}
#endif
