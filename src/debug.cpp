//
//  debug.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "debug.h"

std::atomic<int> lfmLogLevel{static_cast<int>(LfmLogLevel::INFO)};
