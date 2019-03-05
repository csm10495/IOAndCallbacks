// Tests file for IOAndCallbacks
// (C) - csm10495 - MIT License 2019

#include "io.h"
#include "io_lba_generator.h"
#include "iorand.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include <string.h>

#if TEST_BUILD

#ifdef IO_WIN32
#define TEST_PATH "\\\\.\\PHYSICALDRIVE0"
#endif

#ifdef IO_LINUX
#define TEST_PATH "/dev/nvme0n1"
#endif

#define ASSERT(condition, msg) _assert(condition, msg, __func__ , __LINE__)
#define RUN_TEST(func) std::cout << "Running: " << #func << "..."; func(); std::cout << " Pass" << std::endl;

void _assert(bool condition, std::string msg, std::string func, unsigned line)
{
	if (!condition)
	{
		std::cerr << std::endl << "Assertion failed: " << func << "(" << line << "): " << msg << std::endl;
		assert(condition);
	}
}

// Yummy Global Variables
// Tests run synchronously so this makes the callbacks a bit easier to muster.
uint64_t g_blockSize = 0;
uint64_t g_blockCount = 0;
uint64_t g_lba = 0;
uint64_t g_numCallbacks = 0;
void* g_userCallbackData = NULL;
void* g_bufferDataToCompare = NULL;

void testCallback(IO_CALLBACK_STRUCT* pCbStruct)
{
	ASSERT(pCbStruct->numBytesRequested == g_blockCount * g_blockSize, "Num bytes requested did not match");
	ASSERT(pCbStruct->lba == g_lba, "LBA did not match");

	ASSERT(pCbStruct->errorCode == 0, "Nonzero error code in callback");
	if (g_bufferDataToCompare && pCbStruct->operation == IO_OPERATION_READ)
	{
		ASSERT(memcmp(pCbStruct->xferBuffer, g_bufferDataToCompare, (size_t)g_blockSize * (size_t)g_blockCount) == 0, "g_bufferDataToCompare did not match xferBuffer");
	}

	if (g_userCallbackData)
	{
		ASSERT(g_userCallbackData == pCbStruct->userCallbackData, "g_userCallbackData didn't match the userCallbackData");
	}

	g_numCallbacks += 1;
}

char* getRandomBuffer(size_t size, IO* io)
{
	char* buf = (char*)io->getAlignedBuffer(size);
	for (size_t i = 0; i < size; i++)
	{
		buf[i] = rand() % 0xFF;
	}
	return buf; // free me!
}


void test_blocksize_and_blockcount_legit()
{
	IO io(TEST_PATH);
	auto blockSize = io.getBlockSize();
	auto blockCount = io.getBlockCount();

	ASSERT(blockCount > blockSize, "blockCount should be greater than block size");
	ASSERT(blockSize % 512 == 0, "blockSize should be divisible by 512");
	ASSERT(blockCount > 0, "blockCount should be greater than 0");
	ASSERT(blockSize > 0, "blockSize should be greater than 0");
}

void test_write_then_read()
{
	IO io(TEST_PATH);

	g_blockSize = io.getBlockSize();
	g_blockCount = rand() % 128;
	g_bufferDataToCompare = getRandomBuffer((size_t)g_blockSize * (size_t)g_blockCount, &io);
	g_lba = rand() % (io.getBlockCount() - g_blockCount);

	auto oldCallbackCount = g_numCallbacks;
	io.write(g_lba, g_blockCount, g_bufferDataToCompare, NULL, NULL);

	// give 1 second to complete this io at most
	size_t i = 0;
	for (; i < 1000; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (io.poll())
		{
			break;
		}
	}

	ASSERT(oldCallbackCount == g_numCallbacks, "NULL callback should not have led to the numCallbacks going up");
	ASSERT(i < 1000, "Took more than a second to write");

	// read back the data;
	int x = 5;
	g_userCallbackData = (void*)&x;
	io.read(g_lba, g_blockCount, testCallback, &x);

	i = 0;
	for (; i < 1000; i++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		if (io.poll())
		{
			break;
		}
	}

	ASSERT(i < 1000, "Took more than a second to read");

	io.freeAlignedBuffer(g_bufferDataToCompare);
	g_bufferDataToCompare = NULL;
	g_userCallbackData = NULL;
}

void test_aligned_memory()
{
	IO io(TEST_PATH);
	auto blockSize = io.getBlockSize();
	void* buffer = io.getAlignedBuffer(1024*128);
	uint64_t addr = (uint64_t)buffer;
	ASSERT(addr % blockSize == 0, "Buffer was not aligned");
	io.freeAlignedBuffer(buffer);
}

void test_iorand()
{
	// extra braces are to go in / out of scope to kill threads for generation (for test speed only)

	{
		IORand i1;
		IORand i2;

		ASSERT(i1.getRandomNumber<int>() != i2.getRandomNumber<int>(), "Two IORand objects yielded the same random number to start");
	}
	
	{
		IORand i3(5);
		IORand i4(5);

		ASSERT(i3.getRandomNumber<uint64_t>() == i4.getRandomNumber<uint64_t>(), "Same seed did not generate the same numbers");
	}

	{
		IORand i5(7);
		IORand i6(11);
		ASSERT(i5.getRandomNumber<uint64_t>() != i6.getRandomNumber<uint64_t>(), "Different seeds got the same number");
	}

}

void test_io_lba_generator()
{
	std::shared_ptr<IO> io(new IO(TEST_PATH));
	for (uint64_t seed = 0; seed < 1000; seed+=50)
	{
		std::shared_ptr<IORand> ioRand(new IORand(seed));

		uint64_t maxLba = 100;
		uint64_t numBlocks = 4;
		IOLBAGenerator ioLbaGenerator(io, ioRand, maxLba);

		uint64_t numSeqGenerationsPossible = (uint64_t)floor(maxLba / (double)numBlocks);

		for (uint64_t i = 0; i < numSeqGenerationsPossible; i++)
		{
			ASSERT(i * numBlocks == ioLbaGenerator.generateSequentialLba(numBlocks), "Sequential LBA not properly generated");
		}

		try
		{
			ioLbaGenerator.generateSequentialLba(numBlocks);
			ASSERT(false, "runtime_error was not raised when sequential LBA could not be found");
		}
		catch (const std::runtime_error)
		{
			// if we get here, good
		}

		ioLbaGenerator.removeInUseLba(maxLba - numBlocks, numBlocks);
		ASSERT(ioLbaGenerator.generateSequentialLba(numBlocks) == maxLba - numBlocks, "After removal, still couldn't find a max lba");

		// remove half
		ioLbaGenerator.removeInUseLba(maxLba / 2, maxLba / 2);

		// get random lbas
		uint64_t minRandomGenerationsPossible = (maxLba / 2) / numBlocks / 2;
		for (uint64_t i = 0; i < minRandomGenerationsPossible; i++)
		{
			uint64_t lba = ioLbaGenerator.generateRandomLba(numBlocks);
			ASSERT(lba >= maxLba / 2, "Random lba doesn't seem possible for given parameters");
		}
	}
}

int main()
{
	std::cout << "Running tests!" << std::endl;
	auto seed = time(NULL);
	std::cout << "Seed: " << seed << std::endl;
	srand((unsigned)seed);

	RUN_TEST(test_blocksize_and_blockcount_legit);
	RUN_TEST(test_write_then_read);
	RUN_TEST(test_aligned_memory);
	RUN_TEST(test_iorand);
	RUN_TEST(test_io_lba_generator);

	return EXIT_SUCCESS;
}

#endif // TEST_BUILD