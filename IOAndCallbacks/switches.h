// This is home to build switches for various things
// (C) - csm10495 - MIT License 2019

#pragma once

// set to 1 to build a special exe to run tests
#ifndef TEST_BUILD
#define TEST_BUILD 0
#endif // TEST_BUILD

#ifdef __linux__
#define IO_LINUX
#endif // __linux__

#ifdef _WIN32
#define IO_WIN32
#endif // _WIN32