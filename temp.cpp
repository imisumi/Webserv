#include "temp.h"

int main()
{
	Log::trace("This is a trace message with value {}", 10);
	Log::info("This is an info message with value {}", 20);
	Log::debug("This is a debug message with value {}", 30);
	Log::warn("This is a warn message with value {}", 40);
	Log::error("This is an error message with value {}", 50);
	Log::critical("This is a critical message with value {}", 60);


	Log::info

	return 0;
}