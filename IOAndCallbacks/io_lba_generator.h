// IO LBA Generator header file for IO
// (C) - csm10495 - MIT License 2019

#pragma once

#include "io.h"
#include "iorand.h"

#include <memory>
#include <unordered_set>

class IOLBAGenerator
{
public:
	IOLBAGenerator();
	IOLBAGenerator(std::shared_ptr<IO> ioPtr, std::shared_ptr<IORand> ioRand);
	IOLBAGenerator(std::shared_ptr<IO> ioPtr, std::shared_ptr<IORand> ioRand, uint64_t maxLba);

	// gives the next sequential lba
	uint64_t generateSequentialLba(uint64_t blockCount);

	// gives the next random lba
	uint64_t generateRandomLba(uint64_t blockCount);

	// removes an in use lba
	void removeInUseLba(uint64_t lba);
	void removeInUseLba(uint64_t lba, uint64_t blockCount);

	// gets the last generated lba
	uint64_t getLastGeneratedLba() const;

private:

	// returns true if this is a generateable lba/blockcount combo
	bool isGenerateable(uint64_t lba, uint64_t blockCount);

	// saved IO object
	std::shared_ptr<IO> ioPtr;

	// saved rand object
	std::shared_ptr<IORand> ioRand;

	// Last generated LBA
	uint64_t lastGeneratedLba;

	// max generate-able LBA
	uint64_t maxLba;

	// keep track of lbas in use.
	std::unordered_set<uint64_t> inUseLbas;
};
