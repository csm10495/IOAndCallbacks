// IO Rand implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "switches.h"
#include "iorand.h"

#include <cassert>
#include <cstdlib>
#include <ctime>

#if IO_ENABLE_THREADED_RANDOM_GENERATOR
#define MAX_READY_RANDOM_NUMBERS 0x40000
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR

IORand::IORand()
{
	mtGenerator.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
#if IO_ENABLE_THREADED_RANDOM_GENERATOR
	fillAndStartGenerationThread();
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR
}

IORand::IORand(unsigned long long seed)
{
	mtGenerator.seed(seed);
#if IO_ENABLE_THREADED_RANDOM_GENERATOR
	fillAndStartGenerationThread();
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR
}

void IORand::fillBuffer(char * buffer, size_t bufferSize)
{
	assert(bufferSize % sizeof(uint64_t) == 0);

	uint64_t* buf = (uint64_t*)buffer;

	// going 8 at a time is wicked faster than 1 at a time!
	for (size_t i = 0; i < bufferSize / sizeof(uint64_t); i += sizeof(uint64_t))
	{
		*buf = getRandomNumber<uint64_t>();
		buf++;
	}
}

IORand::~IORand()
{
#if IO_ENABLE_THREADED_RANDOM_GENERATOR
	shouldGenerationThreadRun = false;
	randomNumberGenerationThread.join();
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR
}

#if IO_ENABLE_THREADED_RANDOM_GENERATOR
void IORand::fillAndStartGenerationThread()
{
	while (randomNumbers.size() < MAX_READY_RANDOM_NUMBERS)
	{
		randomNumbers.push_back(mtGenerator());
	}

	shouldGenerationThreadRun = true;
	randomNumberGenerationThread = std::thread(&IORand::generateNumbersThread, this);
}

void IORand::generateNumbersThread()
{
	while (shouldGenerationThreadRun)
	{
		size_t idx = randomNumbers.size();
		while (idx < MAX_READY_RANDOM_NUMBERS)
		{
			std::lock_guard<std::mutex>(this->randomNumbersMutex);

			randomNumbers.push_back(mtGenerator());
			idx++;
		}
	}
}
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR
