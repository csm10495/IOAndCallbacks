// IO Rand header file for IO
// (C) - csm10495 - MIT License 2019

#pragma once

#include <atomic>
#include <list>
#include <random>
#include <thread>
#include <mutex>

// IO Rand wraps the included c++11 randomization classes
//  use getRandomNumber() to get an (at most) 64-bit number
class IORand
{
public:
	// Default constructor will attempt to use the high_resolution_clock to get a seed
	IORand();

	// Use the user's given seed to seed number generation
	IORand(unsigned long long seed);

	template <typename T> inline
	T getRandomNumber()
	{
#if IO_ENABLE_THREADED_RANDOM_GENERATOR
		while (true)
		{
			std::lock_guard<std::mutex>(this->randomNumbersMutex);
			if (randomNumbers.size())
			{
				auto val = (T)(*randomNumbers.begin());
				randomNumbers.pop_front();
				return val;
			}

			// if we didn't have a num, this loop should go around
		}

		//throw std::runtime_error("Did not get a random number... somehow broke the loop");
		return 0;
#else
		return (T)mtGenerator();
#endif
	}

	template <typename T> inline
		T getRandomNumber(T start, T end)
	{
		auto r = getRandomNumber<uint64_t>();
		return (T)(r % (end - start)) + start + 1;
	}

	// fill a buffer with random data
	void fillBuffer(char* buffer, size_t bufferSize);

	~IORand();

private:
	// C++11 64-bit generator
	std::mt19937_64 mtGenerator;

#if IO_ENABLE_THREADED_RANDOM_GENERATOR
	// Will fill the initial list to fill up then start thread to keep it filled
	void fillAndStartGenerationThread();

	// To be run in a thread. If we don't have a full list, add one
	void generateNumbersThread();

	// List of random numbers. Added to the end and removed from the start
	std::list<uint64_t> randomNumbers;

	// Mutex for access to randomNumbers list
	std::mutex randomNumbersMutex;

	// Thread to be started by fillAndStartGenerationThread()
	std::thread randomNumberGenerationThread;

	// bool to be set to false when the randomNumberGenerationThread should end
	std::atomic<bool> shouldGenerationThreadRun;
#endif
};
