#ifdef _WIN32

#include "io.h"
#include <iostream>

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
		delete[] pCbStruct->xferBuffer;
	}

	delete lpOverlapped->hEvent;
	delete lpOverlapped;
}

bool io(HANDLE handle, uint64_t lba, uint64_t blockCount, uint32_t blockSize, IO_CALLBACK_FUNCTION* callback, IO_OPERATION_ENUM operation, void* xferData)
{
	uint64_t offsetBytes = blockSize * lba;
	if (!SetFilePointerEx(
		handle,
		*(LARGE_INTEGER*)&offsetBytes,
		NULL,
		FILE_BEGIN
	))
	{
		oserror("Failed to SetFilePointerEx to " + std::to_string(offsetBytes));
		return false;
	}

	OVERLAPPED* pOverlapped = new OVERLAPPED();                                // free this third
	pOverlapped->hEvent = new IO_CALLBACK_STRUCT();                            // free this second
	auto pCbStruct = (IO_CALLBACK_STRUCT*)pOverlapped->hEvent;
	pCbStruct->errorCode = NO_ERROR;
	pCbStruct->lba = lba;
	pCbStruct->numBlocksRequested = blockCount;
	pCbStruct->numBytesRequested = blockCount * blockSize;

	if (operation == IO_OPERATION_READ)
	{
		pCbStruct->xferBuffer = new unsigned char[(unsigned)pCbStruct->numBytesRequested]; // free this first
	}
	else
	{
		pCbStruct->xferBuffer = xferData;
	}

	pCbStruct->userCallbackFunction = callback;
	pCbStruct->operation = operation;

	WIN32_IO_FUNCTION* ioFunction;
	std::string ioFunctionName;
	if (operation == IO_OPERATION_READ)
	{
		ioFunction = (WIN32_IO_FUNCTION*)ReadFileEx;
		ioFunctionName = "ReadFileEx";
	}
	else if (operation == IO_OPERATION_WRITE)
	{
		ioFunction = (WIN32_IO_FUNCTION*)WriteFileEx;
		ioFunctionName = "WriteFileEx";
	}
	else
	{
		oserror("Invalid IO Operation: " + std::to_string(operation));
		goto cleanup;
	}

	if (!ioFunction(
		handle,
		pCbStruct->xferBuffer,
		(DWORD)pCbStruct->numBytesRequested,
		pOverlapped,
		(LPOVERLAPPED_COMPLETION_ROUTINE)overlappedCompletionRoutine
	))
	{
		oserror("Call to " + ioFunctionName + " for an lba of " + std::to_string(lba) + " and a blockcount of " + std::to_string(blockCount) + " failed");
		goto cleanup;
	}

	return true;

cleanup:
	// cleanup
	if (operation == IO_OPERATION_READ)
	{
		delete[] pCbStruct->xferBuffer;
	}

	delete pOverlapped->hEvent;
	delete pOverlapped;

	return false;

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

bool IO::read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback)
{
	return io(handle, lba, blockCount, getBlockSize(), callback, IO_OPERATION_READ, NULL);
}

bool IO::write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback)
{
	return io(handle, lba, blockCount, getBlockSize(), callback, IO_OPERATION_WRITE, xferData);
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

#endif