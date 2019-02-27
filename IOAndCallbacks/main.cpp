// Main entry file for IOAndCallbacks.
// (C) - csm10495 - MIT License 2019

#include "io.h"

#include <iostream>

#if !TEST_BUILD

int numCallbacks = 0;

void testCallback(IO_CALLBACK_STRUCT* pCbStruct)
{
	std::cout << "==========" << std::endl;
	std::cout << pCbStruct->toString() << std::endl;
	std::cout << "==========" << std::endl;
	numCallbacks += 1;
}

int main()
{
#ifdef IO_WIN32
	std::string path = "\\\\.\\PHYSICALDRIVE3";
#elif IO_LINUX
	std::string path = "/dev/nvme0n1";
#endif

	IO io(path);
	char *data = (char*)io.getAlignedBuffer(4096);

	for (size_t i = 0; i < 4096; i++)
	{
		data[i] = i % 0xFF;
	}

	std::cout << "Block Size:  " << io.getBlockSize() << std::endl;
	std::cout << "Block Count: " << io.getBlockCount() << std::endl;

	io.write(0, 8, data, testCallback);
	io.read(0, 8, testCallback);

	while (numCallbacks < 2)
	{
		io.poll();
	}

	io.freeAlignedBuffer(data);

	return EXIT_SUCCESS;
}
#endif // !TEST_BUILD

// todo: allow ability to give a pointer to place data in for reads.
// ... if it is NULL, allocate internally.
// ... if it is passed, do NOT free it for the user