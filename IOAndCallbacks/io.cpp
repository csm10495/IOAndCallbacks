// OS-generalized implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "switches.h"

// Intellisense is messing up without this for some reason
#ifdef IO_WIN32
#include <io.h>
#endif 

#if IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS
// look for boot signature
#define BYTE_510_PARTITION_SECTOR 0x55
#define BYTE_511_PARTITION_SECTOR 0xAA
#endif // IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS

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
	bool result = true;

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
#endif // IO_ENABLE_STATS

#if IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS
	if (ioCallbackStruct->operation == IO_OPERATION_WRITE && !allowWrites)
	{
		fprintf(stderr, "Writes are not currently allowed via this IO object\n");
		result = false;
	}
#endif

	result = result && doSubmitIo(ioCallbackStruct);

#if IO_ENABLE_STATS
	if (!result && ioCallbackStruct->operation == IO_OPERATION_READ)
	{
		ioStatsStruct.NumberOfReadQueueFailures++;
	}
	else if (!result && ioCallbackStruct->operation == IO_OPERATION_WRITE)
	{
		ioStatsStruct.NumberOfWriteQueueFailures++;
	}
#endif // IO_ENABLE_STATS

	return result;
}

#if IO_ENABLE_STATS
IO_STATS_STRUCT& IO::getIoStatsStruct()
{
	return ioStatsStruct;
}
#endif // IO_ENABLE_STATS

#if IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS
void IO::checkAndSetIfWeShouldAllowWrites()
{
	allowWrites = true;

	// do a read to the start of the drive. If it fails, do not allow writes, since we don't know anything
	//   if it passes, check for the AA55 boot signature at the end of the first 512 bytes. If it has that, assume we shouldn't allow writes
	if (read(0, 1, [](IO_CALLBACK_STRUCT* ioInfo) {
		if (ioInfo->succeeded())
		{
			if (((unsigned char*)ioInfo->xferBuffer)[510] == BYTE_510_PARTITION_SECTOR && ((unsigned char*)ioInfo->xferBuffer)[511] == BYTE_511_PARTITION_SECTOR)
			{
				fprintf(stderr, "[callback] Likely partition sector found\n");
				*(bool*)ioInfo->userCallbackData = false;
			}
		}
		else
		{
			// failure... don't allow writes
			fprintf(stderr, "[callback] Unable to read to check for partition sector\n");
			*(bool*)ioInfo->userCallbackData = false;
		}
	}, (void*)&allowWrites))
	{
		while (!poll())
		{
			// busy wait poll. after poll gets true, the callback had happened
		}
	}
	else
	{
		fprintf(stderr, "[queuing] Unable to read to check for partition sector\n");
		allowWrites = false;
	}
}
#endif // IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS
