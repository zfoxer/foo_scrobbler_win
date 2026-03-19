#pragma once

// Avoid Windows min/max macro poisoning
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Windows first
#include <windows.h>
#include <mmsystem.h>

// WTL/ATL stuff
#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

// foobar SDK umbrella (THIS is the important bit)
#include <foobar2000/SDK/foobar2000.h>