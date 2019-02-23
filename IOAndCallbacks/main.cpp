#include "io.h"

#include <iostream>

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
#ifdef _WIN32
	std::string path = "\\\\.\\PHYSICALDRIVE3";
#elif __linux__
	std::string path = "/dev/nvme0n1";
#endif

	IO io(path);
	char *data;
#ifdef __linux__
	posix_memalign((void**)&data, 4096, 4096);
#elif _WIN32
	data = (char*)malloc(4096);
#endif


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

	free(data);

	return EXIT_SUCCESS;
}

// todo: allow ability to give a pointer to place data in for reads.
// ... if it is NULL, allocate internally.
// ... if it is passed, do NOT free it for the user