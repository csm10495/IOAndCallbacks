// This is home to build switches for various things
// (C) - csm10495 - MIT License 2019

#pragma once

// set to 1 to build a special exe to run tests
#ifndef TEST_BUILD
#define TEST_BUILD 0
#endif // TEST_BUILD

// set to 1 to enable the ability for the IO object to keep track of some stats
#ifndef IO_ENABLE_STATS
#define IO_ENABLE_STATS 1
#endif // IO_ENABLE_STATS

// set to 1 to allow a background thread to fill the random number queue.
//  if set to 0, random numbers will be generated on an as-needed basis
#ifndef IO_ENABLE_THREADED_RANDOM_GENERATOR
#define IO_ENABLE_THREADED_RANDOM_GENERATOR 1
#endif // IO_ENABLE_THREADED_RANDOM_GENERATOR

// i have messed with data drives a couple times, its not fun.
//  fail to queue any writes to a drive with a partition
//    ... requires we check for it and block all writes at the queue phase.
// note that IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS is ignored if you call submitIo directly
#ifndef IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS
#define IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS 1
#endif // IO_DISABLE_WRITES_TO_DRIVE_WITH_PARTITIONS

#ifdef __linux__
#define IO_LINUX 1
#endif // __linux__

#ifdef _WIN32
#define IO_WIN32 1
#endif // _WIN32

#ifdef IO_WIN32
#define IO_HANDLE HANDLE
#elif IO_LINUX
#define IO_HANDLE int
#else
#error "Unsupported OS... did not detect __linux__ or _WIN32"
#endif
