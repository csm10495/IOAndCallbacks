// Tests file for IOAndCallbacks
// (C) - csm10495 - MIT License 2019

#include "io.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include <string.h>

#if TEST_BUILD

#ifdef IO_WIN32
#define TEST_PATH "\\\\.\\PHYSICALDRIVE3"
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
	io.write(g_lba, g_blockCount, g_bufferDataToCompare, NULL);

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
	io.read(g_lba, g_blockCount, testCallback);

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

int main()
{
	std::cout << "Running tests!" << std::endl;
	auto seed = time(NULL);
	std::cout << "Seed: " << seed << std::endl;
	srand((unsigned)seed);

	RUN_TEST(test_blocksize_and_blockcount_legit);
	RUN_TEST(test_write_then_read);
	RUN_TEST(test_aligned_memory);

	return EXIT_SUCCESS;
}

#endif // TEST_BUILD