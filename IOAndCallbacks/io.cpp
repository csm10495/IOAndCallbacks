// OS-generalized implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "switches.h"

// Intellisense is messing up without this for some reason
#ifdef IO_WIN32
#include <io.h>
#endif 

#include "io.h"

bool IO::read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback, void* userCallbackData)
{
	auto bytesRequested = blockCount * getBlockSize();

	// free later
	IO_CALLBACK_STRUCT* ioCallbackStruct = new IO_CALLBACK_STRUCT(lba, blockCount, bytesRequested,
		getAlignedBuffer(bytesRequested), IO_OPERATION_READ, callback, userCallbackData);

	return submitIo(ioCallbackStruct);
}

bool IO::write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback, void* userCallbackData)
{
	// free later
	IO_CALLBACK_STRUCT* ioCallbackStruct = new IO_CALLBACK_STRUCT(lba, blockCount, blockCount * getBlockSize(),
		xferData, IO_OPERATION_WRITE, callback, userCallbackData);

	return submitIo(ioCallbackStruct);
}