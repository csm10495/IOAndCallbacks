// Linux implementation file for IO
// (C) - csm10495 - MIT License 2019

#ifdef __linux__

#include "io.h"
#include <iostream>
#include <string>
#include <string.h>

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <signal.h>

#define BREAK_HERE raise(SIGINT)

#define DEFAULT_MAX_EVENTS 0xFFFF

inline int io_setup(unsigned nr, aio_context_t *ctxp)
{
	return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx)
{
	return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr,  struct iocb **iocbpp)
{
 	return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
		struct io_event *events, struct timespec *timeout)
{
	return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

void perror(std::string str)
{
	perror(str.c_str());
}

bool io(int fd, uint64_t lba, uint64_t blockCount, uint32_t blockSize, IO_CALLBACK_FUNCTION* callback,
	IO_OPERATION_ENUM operation, void* xferData,
	aio_context_t aioContext)
{
	iocb* io = new iocb;
	memset(io, 0, sizeof(iocb));

	if (operation == IO_OPERATION_READ)
	{
		io->aio_lio_opcode = IOCB_CMD_PREAD;
	}
	else if (operation == IO_OPERATION_WRITE)
	{
		io->aio_lio_opcode = IOCB_CMD_PWRITE;
	}
	else
	{
		std::cerr << ("Invalid IO Operation: " + std::to_string(operation)) << std::endl;

		delete io;
		return false;
	}

	io->aio_fildes = fd;
	io->aio_nbytes = blockCount * blockSize;
	io->aio_offset = lba * blockSize;

	if (operation == IO_OPERATION_READ)
	{
		io->aio_buf = (__u64)aligned_alloc(blockSize, io->aio_nbytes); // free this first
	}
	else
	{
		io->aio_buf = (__u64)xferData;
	}

	// setup our generic callback
	IO_CALLBACK_STRUCT* callbackStruct = new IO_CALLBACK_STRUCT;
	callbackStruct->errorCode = 0;
	callbackStruct->lba = lba;
	callbackStruct->numBlocksRequested = blockCount;
	callbackStruct->numBytesRequested = io->aio_nbytes;
	callbackStruct->xferBuffer = (void*)io->aio_buf;
	callbackStruct->operation = operation;
	callbackStruct->userCallbackFunction = callback;

	// pass our callback via the kernel
	io->aio_data = (__u64)callbackStruct;

	int ret = io_submit(aioContext, 1, &io);
	if (ret != 1)
	{
		perror("Failed to submit IO");
		delete io;
		delete callbackStruct;

		return false;
	}

	return true;
}

IO::IO(std::string path)
{
	blockSize = 0;
	blockCount = 0;

    fd = open(path.c_str(), O_ASYNC | O_DIRECT | O_RDWR);

	aioContext = 0; // must be pre-initialized
	if (io_setup(DEFAULT_MAX_EVENTS, &aioContext) != 0)
	{
		perror("io_setup() failed");
	}
}

IO::~IO()
{
	if (io_destroy(aioContext) != 0)
	{
		perror("io_destroy failed");
	}

	// finally close the fd;
	close(fd);
	fd = 0;
}

bool IO::read(uint64_t lba, uint64_t blockCount, IO_CALLBACK_FUNCTION* callback)
{
	return io(fd, lba, blockCount, getBlockSize(), callback, IO_OPERATION_READ, NULL, aioContext);
}

bool IO::write(uint64_t lba, uint64_t blockCount, void* xferData, IO_CALLBACK_FUNCTION* callback)
{
	return io(fd, lba, blockCount, getBlockSize(), callback, IO_OPERATION_WRITE, xferData, aioContext);
}

bool IO::poll()
{
	io_event events[DEFAULT_MAX_EVENTS];
	int numEventsCompleted = io_getevents(aioContext, 0, DEFAULT_MAX_EVENTS, events, NULL);

	if (numEventsCompleted < 0)
	{
		perror(std::string("io_getevents returned a negative number: ") + std::to_string(numEventsCompleted));
		return false;
	}

	// now we do the callbacks
	for (int idx = 0; idx < numEventsCompleted; idx++)
	{
		io_event* event = &events[idx];
		IO_CALLBACK_STRUCT* pCbStruct = (IO_CALLBACK_STRUCT*)event->data;
		iocb* io = (iocb*)event->obj;

		// res will be the number of bytes xfer'd or -1 on error
		if (event->res == -1)
		{
			pCbStruct->errorCode = errno;
		}

		pCbStruct->numBytesXferred = event->res;

		if (pCbStruct->userCallbackFunction)
		{
			pCbStruct->userCallbackFunction(pCbStruct);
		}

		// free all allocated
		if (pCbStruct->operation == IO_OPERATION_READ)
		{
			// if doing a write, the user owns the buffer. Let them free it.
			free(pCbStruct->xferBuffer);
		}

		delete pCbStruct;
		delete io;
	}

	return (bool)numEventsCompleted;
}

uint32_t IO::getBlockSize()
{
	// short circuit to only grab once form the OS.
	if (blockSize)
	{
		return blockSize;
	}

	uint64_t numBytes;
	if (ioctl(fd, BLKSSZGET, &numBytes) == 0)
	{
		blockSize = numBytes;
	}
	else
	{
		perror("Couldn't get block size");
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

	uint64_t numBlocks;
	if (ioctl(fd, BLKGETSIZE, &numBlocks) == 0)
	{
		blockCount = numBlocks;
	}
	else
	{
		perror("Couldn't get block count");
	}

	return blockCount;
}

#endif