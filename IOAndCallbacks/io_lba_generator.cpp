// IO LBA Generator implementation file for IO
// (C) - csm10495 - MIT License 2019

#include "io_lba_generator.h"

IOLBAGenerator::IOLBAGenerator()
{
	this->lastGeneratedLba = -1;
}

IOLBAGenerator::IOLBAGenerator(std::shared_ptr<IO> ioPtr, std::shared_ptr<IORand> ioRand) : IOLBAGenerator()
{
	this->ioPtr = ioPtr;
	this->maxLba = ioPtr->getBlockCount() - 1;
	this->ioRand = ioRand;
}

IOLBAGenerator::IOLBAGenerator(std::shared_ptr<IO> ioPtr, std::shared_ptr<IORand> ioRand, uint64_t maxLba) : IOLBAGenerator()
{
	this->ioPtr = ioPtr;
	this->maxLba = maxLba;
	this->ioRand = ioRand;
}

uint64_t IOLBAGenerator::generateSequentialLba(uint64_t blockCount)
{
	this->lastGeneratedLba++;
	if (this->lastGeneratedLba >= maxLba - blockCount)
	{
		this->lastGeneratedLba = 0;
	}

	uint64_t attemptNum = 0;
	while (true)
	{
		if (!isGenerateable(this->lastGeneratedLba, blockCount))
		{
			this->lastGeneratedLba += 1;
		}
		else if (this->lastGeneratedLba > maxLba - blockCount)
		{
			this->lastGeneratedLba = 0;
		}
		else
		{
			break;
		}
		attemptNum++;

		if (attemptNum >= maxLba)
		{
			//throw std::runtime_error("Could not generate sequential LBA");
			return -1;
		}
	}

	// add all of these lbas to the inUseLbas set
	for (uint64_t lba = this->lastGeneratedLba; lba < this->lastGeneratedLba + blockCount; lba++)
	{
		inUseLbas.insert(lba);
	}

	return this->lastGeneratedLba;
}

uint64_t IOLBAGenerator::generateRandomLba(uint64_t blockCount)
{
	this->lastGeneratedLba = ioRand->getRandomNumber<uint64_t>(0, maxLba - blockCount);
	return generateSequentialLba(blockCount);
}

void IOLBAGenerator::removeInUseLba(uint64_t lba)
{
	inUseLbas.erase(lba);
}

void IOLBAGenerator::removeInUseLba(uint64_t lba, uint64_t blockCount)
{
	for (uint64_t i = lba; i < lba + blockCount; i++)
	{
		inUseLbas.erase(i);
	}
}

uint64_t IOLBAGenerator::getLastGeneratedLba() const
{
	return lastGeneratedLba;
}

bool IOLBAGenerator::isGenerateable(uint64_t lba, uint64_t blockCount)
{
	for (uint64_t testLba = lba; testLba < lba + blockCount; testLba++)
	{
		if (inUseLbas.find(testLba) != inUseLbas.end())
		{
			return false;
		}
	}
	return true;
}
