//
//  lastfm_prefs_pane_state.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "lastfm_prefs_pane_state.h"
#include "debug.h"

#include <foobar2000/SDK/foobar2000.h>

#include <mutex>

namespace
{
static const GUID GUID_CFG_LASTFM_PREFS_PANE_RADIO = {
    0x82c20839, 0x9acf, 0x405a, {0x92, 0xd4, 0x39, 0x84, 0xae, 0x7c, 0x4b, 0xd0}};

static const GUID GUID_CFG_LASTFM_PREFS_PANE_CHECKBOX = {
    0x36785a24, 0x92d7, 0x4578, {0xa9, 0xe3, 0x6d, 0xed, 0x09, 0xd3, 0x72, 0xe2}};

// Defaults: radio = middle option, checkbox = off
static cfg_int cfgLastfmPrefsPaneRadio(GUID_CFG_LASTFM_PREFS_PANE_RADIO, 1);
static cfg_bool cfgLastfmPrefsPaneCheckbox(GUID_CFG_LASTFM_PREFS_PANE_CHECKBOX, false);

static std::mutex prefsPaneMutex;

static int clampRadioChoice(int v)
{
    if (v < 0)
        return 0;
    if (v > 2)
        return 2;
    return v;
}
} // namespace

int lastfmGetPrefsPaneRadioChoice()
{
    std::lock_guard<std::mutex> lock(prefsPaneMutex);
    const t_int64 raw = cfgLastfmPrefsPaneRadio.get();
    return clampRadioChoice(static_cast<int>(raw));
}

void lastfmSetPrefsPaneRadioChoice(int value)
{
    value = clampRadioChoice(value);

    std::lock_guard<std::mutex> lock(prefsPaneMutex);
    cfgLastfmPrefsPaneRadio = value;
    LFM_DEBUG("PrefsPane: set radio choice.");
}

bool lastfmGetPrefsPaneCheckbox()
{
    std::lock_guard<std::mutex> lock(prefsPaneMutex);
    return cfgLastfmPrefsPaneCheckbox.get();
}

void lastfmSetPrefsPaneCheckbox(bool enabled)
{
    std::lock_guard<std::mutex> lock(prefsPaneMutex);
    cfgLastfmPrefsPaneCheckbox = enabled;
    LFM_DEBUG("PrefsPane: set checkbox.");
}
