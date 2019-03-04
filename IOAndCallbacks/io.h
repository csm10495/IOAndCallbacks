// Header file for IO
// (C) - csm10495 - MIT License 2019

#pragma once
#include "switches.h"

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <string.h>
#include <unordered_map>

#ifdef IO_WIN32
#define NOMINMAX
#include <Windows.h>
#endif

#ifdef IO_LINUX
// for aio_context_t
#include <linux/aio_abi.h>
#endif

// forward declare
class IO_CALLBACK_STRUCT;

// All IO callbacks follow this format
typedef void(IO_CALLBACK_FUNCTION)(IO_CALLBACK_STRUCT* ioInfo);

typedef enum _IO_OPERATION_ENUM
{
	IO_OPERATION_READ,
	IO_OPERATION_WRITE
} IO_OPERATION_ENUM, *PIO_OPERATION_ENUM;

// structure passed to the callback function
// this is a class since GCC doesn't seem to like constructors in structs
class IO_CALLBACK_STRUCT
{
public:
	IO_CALLBACK_STRUCT()
	{
		memset(this, 0, sizeof(IO_CALLBACK_STRUCT));
	}

	IO_CALLBACK_STRUCT(uint64_t lba, uint64_t blockcount, uint64_t bytesRequested,
		void* xferBuffer, IO_OPERATION_ENUM operation, IO_CALLBACK_FUNCTION* userCallbackFunction,
		void* userCallbackData) : IO_CALLBACK_STRUCT()
	{
		this->lba = lba;
		this->numBlocksRequested = blockcount;
		this->numBytesRequested = bytesRequested;
		this->xferBuffer = xferBuffer;
		this->operation = operation;
		this->userCallbackFunction = userCallbackFunction;
		this->userCallbackData = userCallbackData;
	}

	// set before submitting
	uint64_t lba;
	uint64_t numBlocksRequested;
	uint64_t numBytesRequested;
	void* xferBuffer;
	IO_OPERATION_ENUM operation;
	IO_CALLBACK_FUNCTION* userCallbackFunction;
	void* userCallbackData;

	// set by callback.
	uint64_t numBytesXferred;
	uint32_t errorCode;

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
};

#if IO_ENABLE_STATS
// Used to keep track of some stats about this IO Object
class IO_STATS_STRUCT
{
public:
	IO_STATS_STRUCT()
	{
		clear();
	}

	void clear()
	{
		memset(this, 0, sizeof(IO_STATS_STRUCT));
	}

	uint64_t NumberOfQueuedReads;
	uint64_t NumberOfQueuedWrites;
	uint64_t LargestQueuedReadInSectors;
	uint64_t LargestQueuedWriteInSectors;
	uint64_t LowestQueuedReadLba;
	uint64_t HighestQueuedReadLba;
	uint64_t LowestQueuedWriteLba;
	uint64_t HighestQueuedWriteLba;
	uint64_t NumberOfReadQueueFailures;
	uint64_t NumberOfWriteQueueFailures;
};
#endif

class IO
{
public:
	IO(std::string path);
	~IO();

	// return true if the command is queued
	bool read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback, void* userCallbackData);
	bool write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback, void* userCallbackData);
	inline bool read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback) { return read(lba, blockCount, callback, NULL); }
	inline bool write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback) { return write(lba, blockCount, xferData, callback, NULL); }
	
	// Will free ioCallbackStruct on fail or in the callback on pass. This function is the same on all OSes
	bool submitIo(IO_CALLBACK_STRUCT* ioCallbackStruct);

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

#if IO_ENABLE_STATS
	// Returns a struct of stats
	IO_STATS_STRUCT& getIoStatsStruct();
#endif // IO_ENABLE_STATS

private:

	// called by submitIo(). This is OS specific.
	bool doSubmitIo(IO_CALLBACK_STRUCT* ioCallbackStruct);

	IO_HANDLE handle;

#ifdef IO_LINUX
	aio_context_t aioContext;
#endif

	uint32_t blockSize;
	uint64_t blockCount;

#if IO_ENABLE_STATS
	IO_STATS_STRUCT ioStatsStruct;
#endif // IO_ENABLE_STATS
};
