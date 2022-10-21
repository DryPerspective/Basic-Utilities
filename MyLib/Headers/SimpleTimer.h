#ifndef MYLIBSIMPLETIMER
#define MYLIBSIMPLETIMER
#pragma once

#include <chrono>

namespace utility {
	/*
	* The SimpleTimer class is, as the name suggests, an object which uses the <chrono> header to time code. It is a simple timer because it only keeps track of one point in time
	* and can only return time elapsed relative to that point.
	*/
	class SimpleTimer
	{
	private:
		std::chrono::time_point<std::chrono::steady_clock> m_trackedTime{ std::chrono::steady_clock::now() };	//A chrono object representing the time being tracked.
																												//Because it is member data it will be instantiated with the object.

	public:
		//Reset the clock to the time that this function is called.
		void reset();

		//And return how many seconds have passed since the tracked time.
		double elapsed();

	};
}
#endif