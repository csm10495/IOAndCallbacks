// OS-generalized implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "switches.h"

// Intellisense is messing up without this for some reason
#ifdef IO_WIN32
#include <io.h>
#endif 

#include "io.h"

#include <algorithm>

bool IO::read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback, void* userCallbackData)
{
	auto bytesRequested = blockCount * getBlockSize();

	// free later
	IO_CALLBACK_STRUCT* ioCallbackStruct = new IO_CALLBACK_STRUCT(lba, blockCount, bytesRequested,
		getAlignedBuffer((size_t)bytesRequested), IO_OPERATION_READ, callback, userCallbackData);

	return submitIo(ioCallbackStruct);
}

bool IO::write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback, void* userCallbackData)
{
	// free later
	IO_CALLBACK_STRUCT* ioCallbackStruct = new IO_CALLBACK_STRUCT(lba, blockCount, blockCount * getBlockSize(),
		xferData, IO_OPERATION_WRITE, callback, userCallbackData);

	return submitIo(ioCallbackStruct);
}

bool IO::submitIo(IO_CALLBACK_STRUCT* ioCallbackStruct)
{
#if IO_ENABLE_STATS
	if (ioCallbackStruct->operation == IO_OPERATION_READ)
	{
		ioStatsStruct.NumberOfQueuedReads++;
		ioStatsStruct.LargestQueuedReadInSectors = std::max(ioCallbackStruct->numBlocksRequested, ioStatsStruct.LargestQueuedReadInSectors);
		ioStatsStruct.LowestQueuedReadLba = std::max(ioCallbackStruct->lba, ioStatsStruct.LowestQueuedReadLba);
		ioStatsStruct.HighestQueuedReadLba = std::max(ioCallbackStruct->lba, ioStatsStruct.HighestQueuedReadLba);
	}
	else if (ioCallbackStruct->operation == IO_OPERATION_WRITE)
	{
		ioStatsStruct.NumberOfQueuedWrites++;
		ioStatsStruct.LargestQueuedWriteInSectors = std::max(ioCallbackStruct->numBlocksRequested, ioStatsStruct.LargestQueuedWriteInSectors);
		ioStatsStruct.LowestQueuedWriteLba = std::max(ioCallbackStruct->lba, ioStatsStruct.LowestQueuedWriteLba);
		ioStatsStruct.HighestQueuedWriteLba = std::max(ioCallbackStruct->lba, ioStatsStruct.HighestQueuedWriteLba);
	}

	bool result = doSubmitIo(ioCallbackStruct);
	if (!result && ioCallbackStruct->operation == IO_OPERATION_READ)
	{
		ioStatsStruct.NumberOfReadQueueFailures++;
	}
	else if (!result && ioCallbackStruct->operation == IO_OPERATION_WRITE)
	{
		ioStatsStruct.NumberOfWriteQueueFailures++;
	}

	return result;
#else
	return doSubmitIo(ioCallbackStruct);
#endif
}

#if IO_ENABLE_STATS
IO_STATS_STRUCT& IO::getIoStatsStruct()
{
	return ioStatsStruct;
}
#endif // IO_ENABLE_STATS
