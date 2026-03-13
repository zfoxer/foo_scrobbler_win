//
//  lastfm_prefs_pane.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include <foobar2000/SDK/advconfig.h>
#include <foobar2000/SDK/advconfig_impl.h>
#include <foobar2000/SDK/foobar2000.h>

#include "lastfm_prefs_pane.h"
#include "debug.h"

#include <atomic>

namespace
{
// Preferences → Advanced → Tools → Foo Scrobbler
static const GUID GUID_LASTFM_PREFS_BRANCH = {
    0x8e0aa52b, 0xdd04, 0x4935, {0x92, 0xcd, 0x64, 0xb6, 0xc1, 0x8b, 0x65, 0x2e}};

static const GUID GUID_LASTFM_PREFS_RADIO_0 = {
    0x26fab26e, 0xba49, 0x448f, {0xbd, 0x8c, 0x39, 0x83, 0x30, 0x48, 0xe6, 0x95}};

static const GUID GUID_LASTFM_PREFS_RADIO_1 = {
    0x594acffd, 0x63bb, 0x4193, {0xb6, 0x54, 0x0e, 0x36, 0xb7, 0xe5, 0xb6, 0xd2}};

static const GUID GUID_LASTFM_PREFS_RADIO_2 = {
    0x31393ed7, 0x5821, 0x4901, {0x9a, 0x32, 0x5c, 0xd3, 0x26, 0xa1, 0x66, 0x14}};

static const GUID GUID_LASTFM_PREFS_CHECKBOX_0 = {
    0x9377030d, 0xc480, 0x4322, {0xa7, 0x55, 0x63, 0xfa, 0x5c, 0xa0, 0x03, 0xb0}};

static const GUID GUID_LASTFM_PREFS_CHECKBOX_1 = {
    0xbe82ca73, 0x5083, 0x42ad, {0xaf, 0xcb, 0xf8, 0x53, 0xa5, 0x9c, 0xdd, 0x4b}};

static const GUID GUID_LASTFM_PREFS_DYNAMIC_RADIO_0 = {
    0xef7c67c0, 0xaaf1, 0x4a57, {0xb1, 0x67, 0x60, 0xa8, 0x65, 0x27, 0xa7, 0x32}}; // No dynamic sources

static const GUID GUID_LASTFM_PREFS_DYNAMIC_RADIO_1 = {
    0x91783cdd, 0xc162, 0x499f, {0xb6, 0xf2, 0x23, 0xa3, 0x06, 0xbe, 0xbb, 0x87}}; // NP only

static const GUID GUID_LASTFM_PREFS_DYNAMIC_RADIO_2 = {
    0xa3a0ebc3, 0x2e22, 0x43be, {0x93, 0x32, 0x77, 0x9c, 0x12, 0x46, 0x73, 0xd2}}; // NP & Scrobbling (default)

static const GUID GUID_LASTFM_PREFS_BRANCH_CONSOLE = {
    0x06081a03, 0x7a25, 0x4a1e, {0xa9, 0x19, 0x5e, 0x37, 0xff, 0x52, 0xb0, 0xe9}};

static const GUID GUID_LASTFM_PREFS_BRANCH_SCROBBLING = {
    0xe431a8cd, 0x8443, 0x4009, {0x96, 0x44, 0x91, 0x69, 0x7e, 0x24, 0xa9, 0x6e}};

static const GUID GUID_LASTFM_PREFS_BRANCH_DYNAMIC = {
    0xf1468164, 0x8c16, 0x4ebf, {0x9e, 0x2c, 0xff, 0x82, 0xb2, 0xa5, 0xf1, 0xa1}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING = {
    0x3ed82d09, 0x5c97, 0x48ce, {0x98, 0x84, 0x20, 0x17, 0x67, 0xfa, 0xf7, 0xe9}};

static const GUID GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY = {
    0x0355b218, 0xe822, 0x4186, {0x9a, 0x74, 0xd4, 0x91, 0x00, 0xc2, 0xb0, 0x61}};

static const GUID GUID_LASTFM_PREFS_EXCLUDE_ARTISTS = {
    0xcea981d8, 0xf533, 0x41be, {0x82, 0xd0, 0xfb, 0x29, 0x40, 0xd2, 0x62, 0x52}};

static const GUID GUID_LASTFM_PREFS_EXCLUDE_TITLES = {
    0x3fab6a41, 0xa514, 0x4836, {0x9d, 0x58, 0x67, 0x2b, 0x2e, 0x4b, 0x4a, 0x1e}};

static const GUID GUID_LASTFM_PREFS_TF_ARTIST = {
    0x02ef9422, 0x82ee, 0x457a, {0xb0, 0x1f, 0x71, 0x87, 0x69, 0xe0, 0x3e, 0xcb}};

static const GUID GUID_LASTFM_PREFS_TF_ALBUM_ARTIST = {
    0xc9e0418b, 0xb50d, 0x4873, {0xb6, 0x7c, 0x95, 0xdd, 0x1f, 0x26, 0xbc, 0xe4}};

static const GUID GUID_LASTFM_PREFS_TF_TITLE = {
    0x514a6656, 0xaebc, 0x419f, {0x84, 0x05, 0x2f, 0xf4, 0x26, 0xb0, 0x8f, 0x61}};

static const GUID GUID_LASTFM_PREFS_TF_ALBUM = {
    0x42c4e618, 0x0143, 0x4025, {0x81, 0xd2, 0x3c, 0x04, 0xfe, 0x11, 0xc7, 0xe9}};

// Branches
static advconfig_branch_factory g_lastfmPrefsBranchFactory("Foo Scrobbler", GUID_LASTFM_PREFS_BRANCH,
                                                           advconfig_branch::guid_branch_tools, -50);

static advconfig_branch_factory g_lastfmPrefsConsoleBranchFactory("Console info", GUID_LASTFM_PREFS_BRANCH_CONSOLE,
                                                                  GUID_LASTFM_PREFS_BRANCH, 0);

static advconfig_branch_factory g_lastfmPrefsTagFormattingBranchFactory("Tag formatting",
                                                                        GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING,
                                                                        GUID_LASTFM_PREFS_BRANCH, 1);

static advconfig_branch_factory g_lastfmPrefsScrobblingBranchFactory("Scrobbling", GUID_LASTFM_PREFS_BRANCH_SCROBBLING,
                                                                     GUID_LASTFM_PREFS_BRANCH, 2);

static advconfig_branch_factory g_lastfmPrefsDynamicBranchFactory("Dynamic sources (overridden by library-only)",
                                                                  GUID_LASTFM_PREFS_BRANCH_DYNAMIC,
                                                                  GUID_LASTFM_PREFS_BRANCH, 3);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagCheckboxTreatVA("Treat \"Various Artists\" as empty (Album Artist only)", GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY,
                         GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING, 4.0, false, false, 0);

static service_factory_single_t<advconfig_entry_string_impl> g_tagArtistTf("Artist (Title Formatting)",
                                                                           GUID_LASTFM_PREFS_TF_ARTIST,
                                                                           GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING, 0.0,
                                                                           "[%ARTIST%]", 0);

static service_factory_single_t<advconfig_entry_string_impl> g_tagAlbumArtistTf("Album Artist (Title Formatting)",
                                                                                GUID_LASTFM_PREFS_TF_ALBUM_ARTIST,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING,
                                                                                1.0, "[%ALBUM ARTIST%]", 0);

static service_factory_single_t<advconfig_entry_string_impl> g_tagTitleTf("Title (Title Formatting)",
                                                                          GUID_LASTFM_PREFS_TF_TITLE,
                                                                          GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING, 2.0,
                                                                          "[%TITLE%]", 0);

static service_factory_single_t<advconfig_entry_string_impl> g_tagAlbumTf("Album (Title Formatting)",
                                                                          GUID_LASTFM_PREFS_TF_ALBUM,
                                                                          GUID_LASTFM_PREFS_BRANCH_TAG_FORMATTING, 3.0,
                                                                          "[%ALBUM%]", 0);

static bool advGetCheckboxState(const GUID& g)
{
    service_ptr_t<advconfig_entry_checkbox> e;
    if (!advconfig_entry::g_find_t(e, g))
        return false;
    return e->get_state();
}

static void advSetCheckboxState(const GUID& g, bool v)
{
    service_ptr_t<advconfig_entry_checkbox> e;
    if (!advconfig_entry::g_find_t(e, g))
        return;
    e->set_state(v);
}

// Radios
static service_factory_single_t<advconfig_entry_checkbox_impl> g_radio0("None", GUID_LASTFM_PREFS_RADIO_0,
                                                                        GUID_LASTFM_PREFS_BRANCH_CONSOLE, 0.0, false,
                                                                        true, // isRadio
                                                                        0     // flags
);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_radio1("Basic", GUID_LASTFM_PREFS_RADIO_1, GUID_LASTFM_PREFS_BRANCH_CONSOLE, 1.0, true, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_radio2("Debug", GUID_LASTFM_PREFS_RADIO_2, GUID_LASTFM_PREFS_BRANCH_CONSOLE, 2.0, false, true, 0);

// Checkboxes, defaults: no, no
static service_factory_single_t<advconfig_entry_checkbox_impl> g_checkbox0("Disable NowPlaying notifications",
                                                                           GUID_LASTFM_PREFS_CHECKBOX_0,
                                                                           GUID_LASTFM_PREFS_BRANCH_SCROBBLING, 0.0,
                                                                           false, false, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_checkbox1("Only scrobble from media library",
                                                                           GUID_LASTFM_PREFS_CHECKBOX_1,
                                                                           GUID_LASTFM_PREFS_BRANCH_SCROBBLING, 1.0,
                                                                           false, false, 0);

// Dynamic sources (3-choice radio group)
static service_factory_single_t<advconfig_entry_checkbox_impl> g_dynamicRadio0("No dynamic sources",
                                                                               GUID_LASTFM_PREFS_DYNAMIC_RADIO_0,
                                                                               GUID_LASTFM_PREFS_BRANCH_DYNAMIC, 0.0,
                                                                               false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_dynamicRadio1("Only NP notifications",
                                                                               GUID_LASTFM_PREFS_DYNAMIC_RADIO_1,
                                                                               GUID_LASTFM_PREFS_BRANCH_DYNAMIC, 1.0,
                                                                               false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_dynamicRadio2("NP & Scrobbling",
                                                                               GUID_LASTFM_PREFS_DYNAMIC_RADIO_2,
                                                                               GUID_LASTFM_PREFS_BRANCH_DYNAMIC, 2.0,
                                                                               true, true, 0);

static service_factory_single_t<advconfig_entry_string_impl>
    g_excludeArtists("Exclude artists (text or regex; ';' separated)", GUID_LASTFM_PREFS_EXCLUDE_ARTISTS,
                     GUID_LASTFM_PREFS_BRANCH_SCROBBLING, 2.0, "", 0);

static service_factory_single_t<advconfig_entry_string_impl>
    g_excludeTitles("Exclude titles (text or regex; ';' separated)", GUID_LASTFM_PREFS_EXCLUDE_TITLES,
                    GUID_LASTFM_PREFS_BRANCH_SCROBBLING, 3.0, "", 0);

static void enforceOneOfN(const GUID* ids, std::size_t n, std::size_t defaultIndex)
{
    std::size_t firstOn = n; // "none"
    std::size_t onCount = 0;

    for (std::size_t i = 0; i < n; ++i)
    {
        if (advGetCheckboxState(ids[i]))
        {
            if (firstOn == n)
                firstOn = i;
            ++onCount;
        }
    }

    // If none ON -> select default.
    if (onCount == 0)
    {
        for (std::size_t i = 0; i < n; ++i)
            advSetCheckboxState(ids[i], i == defaultIndex);
        return;
    }

    // If multiple ON -> keep the first one we saw, clear the rest.
    if (onCount > 1)
    {
        for (std::size_t i = 0; i < n; ++i)
            advSetCheckboxState(ids[i], i == firstOn);
    }
}

static void enforceOneOf3(const GUID& g0, const GUID& g1, const GUID& g2, std::size_t defaultIndex)
{
    const GUID ids[3] = {g0, g1, g2};
    enforceOneOfN(ids, 3, defaultIndex);
}

static void ensureRadioDefaultAdv()
{
    // Default = Basic (index 1)
    enforceOneOf3(GUID_LASTFM_PREFS_RADIO_0, GUID_LASTFM_PREFS_RADIO_1, GUID_LASTFM_PREFS_RADIO_2, 1);
}

static int getConsoleRadioChoice()
{
    ensureRadioDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_PREFS_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_PREFS_RADIO_1))
        return 1;
    return 0;
}

static void ensureDynamicRadioDefaultAdv()
{
    // Default = NP & Scrobbling (index 2)
    enforceOneOf3(GUID_LASTFM_PREFS_DYNAMIC_RADIO_0, GUID_LASTFM_PREFS_DYNAMIC_RADIO_1,
                  GUID_LASTFM_PREFS_DYNAMIC_RADIO_2, 2);
}

static int getDynamicSourcesMode()
{
    ensureDynamicRadioDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_PREFS_CHECKBOX_1))
    {
        static std::atomic<bool> logged{false};
        if (!logged.exchange(true))
            LFM_DEBUG("Dynamic sources: overridden to 'No dynamic sources' because Only-from-library is enabled.");
        return 0;
    }

    if (advGetCheckboxState(GUID_LASTFM_PREFS_DYNAMIC_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_PREFS_DYNAMIC_RADIO_1))
        return 1;
    return 0;
}

static std::string advGetStringState(const GUID& g)
{
    service_ptr_t<advconfig_entry_string> e;
    if (!advconfig_entry::g_find_t(e, g))
        return {};

    pfc::string8 v;
    e->get_state(v);
    return std::string(v.c_str());
}
} // namespace

void lastfmSyncLogLevelFromPrefs()
{
    const int choice = getConsoleRadioChoice();

    const int desired = (choice == 0)   ? static_cast<int>(LfmLogLevel::OFF)
                        : (choice == 1) ? static_cast<int>(LfmLogLevel::INFO)
                                        : static_cast<int>(LfmLogLevel::DEBUG_LOG);

    lfmLogLevel.store(desired);
}

void lastfmRegisterPrefsPane()
{
    // Force sync once at startup, so atomic matches prefs immediately.
    lastfmSyncLogLevelFromPrefs();

    pfc::string_formatter f;
    f << "PrefsPane: Advanced prefs registered. consoleChoice=" << getConsoleRadioChoice()
      << " logLevel=" << lfmLogLevel.load();
    LFM_DEBUG(f.c_str());
}

bool lastfmOnlyScrobbleFromMediaLibrary()
{
    return advGetCheckboxState(GUID_LASTFM_PREFS_CHECKBOX_1);
}

int lastfmDynamicSourcesMode()
{
    return getDynamicSourcesMode();
}

bool lastfmDisableNowPlaying()
{
    return advGetCheckboxState(GUID_LASTFM_PREFS_CHECKBOX_0);
}

std::string lastfmExcludedArtistsPatternList()
{
    return advGetStringState(GUID_LASTFM_PREFS_EXCLUDE_ARTISTS);
}

std::string lastfmExcludedTitlesPatternList()
{
    return advGetStringState(GUID_LASTFM_PREFS_EXCLUDE_TITLES);
}

std::string lastfmArtistTf()
{
    return advGetStringState(GUID_LASTFM_PREFS_TF_ARTIST);
}

std::string lastfmAlbumArtistTf()
{
    return advGetStringState(GUID_LASTFM_PREFS_TF_ALBUM_ARTIST);
}

std::string lastfmTitleTf()
{
    return advGetStringState(GUID_LASTFM_PREFS_TF_TITLE);
}

std::string lastfmAlbumTf()
{
    return advGetStringState(GUID_LASTFM_PREFS_TF_ALBUM);
}

bool lastfmTagTreatVariousArtistsAsEmpty()
{
    return advGetCheckboxState(GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY);
}
