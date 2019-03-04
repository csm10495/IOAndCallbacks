// Win32 implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "io.h"
#include <iostream>

#ifdef IO_WIN32

typedef BOOL(WINAPI WIN32_IO_FUNCTION)(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

void oserror(std::string s)
{
	std::cerr << s << ": OS Error: " << GetLastError() << std::endl;
}

void overlappedCompletionRoutine(
	DWORD dwErrorCode,
	DWORD dwNumberOfBytesTransfered,
	OVERLAPPED* lpOverlapped
)
{
	IO_CALLBACK_STRUCT* pCbStruct = (IO_CALLBACK_STRUCT*)lpOverlapped->hEvent;

	pCbStruct->errorCode = dwErrorCode;
	pCbStruct->numBytesXferred = dwNumberOfBytesTransfered;

	if (pCbStruct->userCallbackFunction)
	{
		pCbStruct->userCallbackFunction(pCbStruct);
	}

	// free all allocated
	if (pCbStruct->operation == IO_OPERATION_READ)
	{
		// if doing a write, the user owns the buffer. Let them free it.
		IO::freeAlignedBuffer(pCbStruct->xferBuffer);
	}

	delete lpOverlapped->hEvent;
	delete lpOverlapped;
}

IO::IO(std::string path)
{
	blockSize = 0;
	blockCount = 0;

	handle = CreateFile(path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL
	);
}

IO::~IO()
{
	// Try to stop all IO issued by our thread.
	CancelIo(handle);

	while (poll())
	{
		// try to finish all IOs
	}

	CloseHandle(handle);
	handle = INVALID_HANDLE_VALUE;
}

bool IO::poll()
{
	return (SleepEx(0, true) == WAIT_IO_COMPLETION);
}

uint32_t IO::getBlockSize()
{
	// short circuit to only grab once form the OS.
	if (blockSize)
	{
		return blockSize;
	}

	STORAGE_PROPERTY_QUERY query;
	query.PropertyId = StorageAccessAlignmentProperty;
	query.QueryType = PropertyStandardQuery;
	STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment;

	ULONG bytesReturned;
	bool ret = DeviceIoControl(
		handle,
		IOCTL_STORAGE_QUERY_PROPERTY,
		&query,
		sizeof(query),
		&alignment,
		sizeof(alignment),
		&bytesReturned,
		NULL
	);

	if (ret)
	{
		blockSize = alignment.BytesPerLogicalSector;
	}

	return blockSize;
}

uint64_t IO::getBlockCount()
{
	// short circuit to only grab once form the OS.
	if (blockCount)
	{
		return blockCount;
	}

	DISK_GEOMETRY_EX geo = { 0 };
	ULONG bytesReturned;
	bool ret = DeviceIoControl(
		handle,
		IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
		&geo,
		sizeof(geo),
		&geo,
		sizeof(geo),
		&bytesReturned,
		NULL
	);

	if (ret)
	{
		blockCount = geo.DiskSize.QuadPart / getBlockSize();
	}

	return blockCount;
}

void* IO::getAlignedBuffer(size_t size)
{
	return _aligned_malloc(size, getBlockSize());
}

void IO::freeAlignedBuffer(void * buffer)
{
	_aligned_free(buffer);
}

bool IO::doSubmitIo(IO_CALLBACK_STRUCT* ioCallbackStruct)
{
	OVERLAPPED* pOverlapped = new OVERLAPPED();                                // free this third
	pOverlapped->hEvent = ioCallbackStruct;                                    // free this second (allocated upstream)

	uint64_t offsetBytes = getBlockSize() * ioCallbackStruct->lba;
	pOverlapped->Offset = offsetBytes & 0xFFFFFFFF;
	pOverlapped->OffsetHigh = offsetBytes >> 32;

	WIN32_IO_FUNCTION* ioFunction;
	std::string ioFunctionName;
	if (ioCallbackStruct->operation == IO_OPERATION_READ)
	{
		ioFunction = (WIN32_IO_FUNCTION*)ReadFileEx;
		ioFunctionName = "ReadFileEx";
	}
	else if (ioCallbackStruct->operation == IO_OPERATION_WRITE)
	{
		ioFunction = (WIN32_IO_FUNCTION*)WriteFileEx;
		ioFunctionName = "WriteFileEx";
	}
	else
	{
		oserror("Invalid IO Operation: " + std::to_string(ioCallbackStruct->operation));
		goto cleanup;
	}

	if (!ioFunction(
		handle,
		ioCallbackStruct->xferBuffer,
		(DWORD)ioCallbackStruct->numBytesRequested,
		pOverlapped,
		(LPOVERLAPPED_COMPLETION_ROUTINE)overlappedCompletionRoutine
	))
	{
		oserror("Call to " + ioFunctionName + " for an lba of " + std::to_string(ioCallbackStruct->lba) + \
			" and a blockcount of " + std::to_string(ioCallbackStruct->numBlocksRequested) + " failed");
		goto cleanup;
	}

	return true;

cleanup:
	// cleanup
	if (ioCallbackStruct->operation == IO_OPERATION_READ)
	{
		delete[] ioCallbackStruct->xferBuffer;
	}

	delete pOverlapped->hEvent;
	delete pOverlapped;

	return false;
}

#endif