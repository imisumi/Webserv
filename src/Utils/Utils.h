


#pragma once

#include <chrono>
#include <iostream>


namespace Utils
{
	class Timer
	{
	public:
		Timer() { reset(); }
		~Timer()
		{
			std::cout << "Elapsed time: " << elapsedMillis() << "ms\n";
		}
		void reset() { start = std::chrono::high_resolution_clock::now(); }
		
		// elapsed time in microseconds
		float elapsed() const { return std::chrono::duration<float, std::micro>(std::chrono::high_resolution_clock::now() - start).count(); }
		// elapsed time in milliseconds
		float elapsedMillis() const { return std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - start).count(); }
		// elapsed time in seconds
		float elapsedSeconds() const { return std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count(); }
	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
	};
}