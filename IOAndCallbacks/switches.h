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
#endif