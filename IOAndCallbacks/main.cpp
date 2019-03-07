// Main entry file for IOAndCallbacks.
// (C) - csm10495 - MIT License 2019

#include "io.h"
#include "io_lba_generator.h"
#include "iorand.h"

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
	std::string path = "\\\\.\\PHYSICALDRIVE0";
#elif IO_LINUX
	std::string path = "/dev/nvme0n1";
#endif

	std::shared_ptr<IO> io(new IO(path));
	std::shared_ptr<IORand> ioRand(new IORand(1));
	char *data = (char*)io->getAlignedBuffer(4096*256);

	for (size_t i = 0; i < 4096; i++)
	{
		data[i] = i % 0xFF;
	}

	std::cout << "Block Size:  " << io->getBlockSize() << std::endl;
	std::cout << "Block Count: " << io->getBlockCount() << std::endl;

	/////////////////////////////	

	for (auto i = 0; i < 8; i++)
	{
		io->write(0, 256, data, testCallback, NULL);
		io->read(0, 8, testCallback, NULL);
	}

	for (auto i = 0; i < 1; i++)
	{
		io->poll();
	}

	io->freeAlignedBuffer(data);
	
	return EXIT_SUCCESS;
}

#endif // !TEST_BUILD

// todo: allow ability to give a pointer to place data in for reads.
// ... if it is NULL, allocate internally.
// ... if it is passed, do NOT free it for the user