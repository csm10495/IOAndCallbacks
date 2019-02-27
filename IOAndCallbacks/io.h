// Header file for IO
// (C) - csm10495 - MIT License 2019

#pragma once
#include "switches.h"

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#ifdef IO_WIN32
#include <Windows.h>
#endif

#ifdef IO_LINUX
// for aio_context_t
#include <linux/aio_abi.h>
#endif

// forward declare
struct _IO_CALLBACK_STRUCT;

// All IO callbacks follow this format
typedef void(IO_CALLBACK_FUNCTION)(_IO_CALLBACK_STRUCT* ioInfo);

typedef enum _IO_OPERATION_ENUM
{
	IO_OPERATION_READ,
	IO_OPERATION_WRITE
} IO_OPERATION_ENUM, *PIO_OPERATION_ENUM;

// structure passed to the callback function
typedef struct _IO_CALLBACK_STRUCT
{
	uint32_t errorCode;
	uint64_t lba;
	uint64_t numBlocksRequested;
	uint64_t numBytesRequested;

	void* xferBuffer;
	uint64_t numBytesXferred;
	IO_OPERATION_ENUM operation;

	IO_CALLBACK_FUNCTION* userCallbackFunction;

	bool failed() const
	{
		return !succeeded();
	}

	bool succeeded() const
	{
		return errorCode == 0 && numBytesRequested == numBytesXferred;
	}

	std::string toString() const
	{
		std::string retString = "";
		retString += "errorCode:           " + std::to_string(errorCode) + "\n";
		retString += "lba:                 " + std::to_string(lba) + "\n";
		retString += "numBlocksRequested:  " + std::to_string(numBlocksRequested) + "\n";
		retString += "numBytesRequested:   " + std::to_string(numBytesRequested) + "\n";
		retString += "numBytesXferred:     " + std::to_string(numBytesXferred) + "\n";
		retString += "operation:           " + std::to_string(operation) + " ";
		if (operation == IO_OPERATION_READ)
		{
			retString += "(IO_OPERATION_READ)";
		}
		else if (operation == IO_OPERATION_WRITE)
		{
			retString += "(IO_OPERATION_WRITE)";
		}
		else
		{
			retString += "(Unknown)";
		}
		retString += "\n";

		return retString;
	}

}IO_CALLBACK_STRUCT, *PIO_CALLBACK_STRUCT;

class IO
{
public:
	IO(std::string path);
	~IO();

	// return true if the command is queued
	bool read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback);
	bool write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback);

	// returns true if at least one callback is called
	bool poll();

	// will ask the OS for the block size once then cache it after
	uint32_t getBlockSize();

	// will ask the OS for the number of blocks once then cache it after
	uint64_t getBlockCount();

	// Will attempt to get a buffer of the requested size that is aligned to the device's block size
	void* getAlignedBuffer(size_t size);

	// Will free an allocated-aligned buffer
	static void freeAlignedBuffer(void* buffer);

private:
#ifdef IO_WIN32
	HANDLE handle;
#else
	int fd;
	aio_context_t aioContext;
#endif

	uint32_t blockSize;
	uint64_t blockCount;
};
