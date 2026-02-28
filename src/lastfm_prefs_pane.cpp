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
    0xa3a0ebc3, 0x2e22, 0x43be, {0x93, 0x32, 0x77, 0x9c, 0x12, 0x46, 0x73, 0xd2}};

static const GUID GUID_LASTFM_PREFS_BRANCH_CONSOLE = {
    0x06081a03, 0x7a25, 0x4a1e, {0xa9, 0x19, 0x5e, 0x37, 0xff, 0x52, 0xb0, 0xe9}};

static const GUID GUID_LASTFM_PREFS_BRANCH_SCROBBLING = {
    0xe431a8cd, 0x8443, 0x4009, {0x96, 0x44, 0x91, 0x69, 0x7e, 0x24, 0xa9, 0x6e}};

static const GUID GUID_LASTFM_PREFS_BRANCH_DYNAMIC = {
    0xf1468164, 0x8c16, 0x4ebf, {0x9e, 0x2c, 0xff, 0x82, 0xb2, 0xa5, 0xf1, 0xa1}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING = {
    0x3cf76502, 0xec87, 0x4837, {0x80, 0x76, 0x8e, 0x8d, 0x4d, 0x0e, 0xd6, 0x8e}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST = {
    0xac21bc9a, 0xa546, 0x4ccf, {0x8c, 0xfa, 0x63, 0xe1, 0xe0, 0x06, 0xc4, 0xd7}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST = {
    0xf0cfd50c, 0x1eee, 0x48b7, {0xbc, 0x6f, 0x4d, 0xf0, 0x20, 0xa8, 0x6c, 0x70}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_TITLE = {
    0x15d85296, 0x6063, 0x4a83, {0x9c, 0x21, 0x0a, 0x60, 0x5d, 0x14, 0x74, 0x9a}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM = {
    0x6b0fb704, 0xaf53, 0x40d8, {0x8c, 0x9d, 0xb9, 0x2a, 0x51, 0x2f, 0xb6, 0xb9}};

static const GUID GUID_LASTFM_PREFS_BRANCH_TAG_FALLBACK_COMP = {
    0x943f1945, 0x2bad, 0x42be, {0xb1, 0xad, 0xe4, 0xa6, 0xd1, 0xde, 0xcc, 0xd0}};

// Artist source radios (5-choice group)
static const GUID GUID_LASTFM_TAG_ARTIST_RADIO_0 = {
    0x7d44d4a9, 0x0e88, 0x405f, {0x84, 0x74, 0xda, 0x98, 0xa5, 0x6f, 0x6d, 0xeb}};

static const GUID GUID_LASTFM_TAG_ARTIST_RADIO_1 = {
    0xb8f63152, 0x8bb7, 0x4368, {0xa9, 0x50, 0xb6, 0xe2, 0x27, 0x1c, 0x39, 0x89}};

static const GUID GUID_LASTFM_TAG_ARTIST_RADIO_2 = {
    0x05ed08a2, 0x9882, 0x427c, {0x8e, 0xf6, 0x68, 0x37, 0x26, 0x82, 0x2a, 0xb8}}; /* PERFORMER */

static const GUID GUID_LASTFM_TAG_ARTIST_RADIO_3 = {
    0x76532e0e, 0xe360, 0x4efb, {0x8d, 0x9c, 0x8f, 0xc1, 0x90, 0x1c, 0x2b, 0x49}}; /* COMPOSER */

static const GUID GUID_LASTFM_TAG_ARTIST_RADIO_4 = {
    0xc7f16657, 0x6909, 0x4ce9, {0xa5, 0x7b, 0x1d, 0xc3, 0xee, 0x68, 0xff, 0x9e} /* CONDUCTOR */};

static const GUID GUID_LASTFM_TAG_CHECKBOX_FALLBACK_ARTIST_ALBUM = {
    0xf014ec61, 0x421a, 0x46e6, {0x81, 0x2f, 0xbb, 0x2b, 0xaf, 0x1e, 0xdf, 0x4c}};

static const GUID GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY = {
    0x0355b218, 0xe822, 0x4186, {0x9a, 0x74, 0xd4, 0x91, 0x00, 0xc2, 0xb0, 0x61}};

static const GUID GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_0 = {
    0xd1033d86, 0xcf49, 0x4f76, {0xbd, 0xa0, 0xc0, 0x20, 0x3e, 0x78, 0x0d, 0xf6}};

static const GUID GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_1 = {
    0xf582b48e, 0x93a4, 0x49ac, {0x8f, 0x43, 0xa3, 0xe3, 0xdd, 0x84, 0x25, 0x59}};

static const GUID GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_2 = {
    0x0f45c024, 0x47f5, 0x48d7, {0x97, 0xba, 0x0d, 0x82, 0xf8, 0xe7, 0x88, 0xff}};

static const GUID GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_3 = {
    0x09fa31dc, 0xb12b, 0x42b2, {0x98, 0xd1, 0x7e, 0x00, 0xf0, 0x32, 0x24, 0xa9}};

static const GUID GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_4 = {
    0x4b2ad97a, 0x1309, 0x494e, {0x88, 0xe1, 0x91, 0x99, 0xe7, 0xd7, 0xa5, 0xf5}};

static const GUID GUID_LASTFM_TAG_TITLE_RADIO_0 = {
    /* TITLE (default) */ 0xe7adbe25, 0xc202, 0x4e20, {0xa6, 0x87, 0xdb, 0xba, 0xbb, 0x1f, 0x1c, 0x9e}};

static const GUID GUID_LASTFM_TAG_TITLE_RADIO_1 = {
    /* WORK */ 0x06b53e24, 0xc4db, 0x4180, {0x97, 0x5b, 0xe1, 0x94, 0xf5, 0x20, 0x25, 0x0f}};

static const GUID GUID_LASTFM_TAG_TITLE_RADIO_2 = {
    /* MOVEMENT */ 0xfbebcc4b, 0x1997, 0x4d28, {0xaf, 0x00, 0x4a, 0x21, 0xa1, 0x9c, 0x9e, 0x1a}};

static const GUID GUID_LASTFM_TAG_TITLE_RADIO_3 = {
    /* PART */ 0x0605d120, 0x3fc0, 0x406a, {0x8f, 0x56, 0x7a, 0x42, 0x03, 0xb1, 0x2c, 0x69}};

static const GUID GUID_LASTFM_TAG_TITLE_RADIO_4 = {
    /* SUBTITLE */ 0x823027e2, 0x36ba, 0x4ab2, {0x9a, 0x8d, 0xb2, 0x04, 0x6b, 0xce, 0xf7, 0x26}};

static const GUID GUID_LASTFM_TAG_ALBUM_RADIO_0 = {
    /* ALBUM (default) */ 0xf1183597, 0x32b0, 0x4d50, {0x96, 0xe2, 0xca, 0x85, 0x2a, 0x6d, 0x01, 0x2f}};

static const GUID GUID_LASTFM_TAG_ALBUM_RADIO_1 = {
    /* RELEASE */ 0x56133aed, 0x58e9, 0x4cde, {0xba, 0x61, 0x47, 0xbb, 0x60, 0x07, 0xae, 0x0a}};

static const GUID GUID_LASTFM_TAG_ALBUM_RADIO_2 = {
    /* WORK */ 0x460efac9, 0xadaa, 0x4a52, {0x9a, 0x37, 0x76, 0x99, 0x7b, 0xf2, 0x18, 0x70}};

static const GUID GUID_LASTFM_TAG_ALBUM_RADIO_3 = {
    /* ALBUMTITLE */ 0x40fe732c, 0xc484, 0x421d, {0xb9, 0xad, 0xef, 0x54, 0x0e, 0xce, 0x61, 0xf6}};

static const GUID GUID_LASTFM_TAG_ALBUM_RADIO_4 = {
    /* DISCNAME */ 0x3329a82a, 0x4ea8, 0x461c, {0xba, 0x75, 0x7c, 0x09, 0x55, 0xda, 0x5a, 0xa4}};

// Branches
static advconfig_branch_factory g_lastfmPrefsBranchFactory("Foo Scrobbler", GUID_LASTFM_PREFS_BRANCH,
                                                           advconfig_branch::guid_branch_tools, -50);

static advconfig_branch_factory g_lastfmPrefsConsoleBranchFactory("Console info", GUID_LASTFM_PREFS_BRANCH_CONSOLE,
                                                                  GUID_LASTFM_PREFS_BRANCH, 0);

static advconfig_branch_factory g_lastfmPrefsScrobblingBranchFactory("Scrobbling", GUID_LASTFM_PREFS_BRANCH_SCROBBLING,
                                                                     GUID_LASTFM_PREFS_BRANCH, 1);

static advconfig_branch_factory g_lastfmPrefsDynamicBranchFactory("Dynamic sources (overridden by library-only)",
                                                                  GUID_LASTFM_PREFS_BRANCH_DYNAMIC,
                                                                  GUID_LASTFM_PREFS_BRANCH, 2);

static advconfig_branch_factory g_lastfmPrefsTagHandlingBranchFactory("Tag Handling",
                                                                      GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING,
                                                                      GUID_LASTFM_PREFS_BRANCH,
                                                                      3 // after Dynamic sources
);

static advconfig_branch_factory g_tagArtistBranch("Artist source", GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                  GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING, 0);

static advconfig_branch_factory g_tagAlbumArtistBranch("Album Artist source", GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST,
                                                       GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING, 1);

static advconfig_branch_factory g_tagTitleBranch("Title source", GUID_LASTFM_PREFS_BRANCH_TAG_TITLE,
                                                 GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING, 2);

static advconfig_branch_factory g_tagAlbumBranch("Album source", GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM,
                                                 GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING, 3);

static advconfig_branch_factory g_tagFallbackCompBranch("Fallback & Compilation Handling",
                                                        GUID_LASTFM_PREFS_BRANCH_TAG_FALLBACK_COMP,
                                                        GUID_LASTFM_PREFS_BRANCH_TAG_HANDLING, 4);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumArtistRadio0("ALBUM ARTIST", GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_0,
                           GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST, 0.0, true, true, 0); // default ON

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumArtistRadio1("ARTIST", GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_1, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST,
                           1.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumArtistRadio2("PERFORMER", GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_2, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST,
                           2.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumArtistRadio3("COMPOSER", GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_3, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST,
                           3.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumArtistRadio4("CONDUCTOR", GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_4, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM_ARTIST,
                           4.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagCheckboxFallback("If selected Artist or Album source is empty, fall back to ARTIST / ALBUM",
                          GUID_LASTFM_TAG_CHECKBOX_FALLBACK_ARTIST_ALBUM, GUID_LASTFM_PREFS_BRANCH_TAG_FALLBACK_COMP,
                          0.0, true, false, 0); // default ON

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagCheckboxTreatVA("Treat \"Various Artists\" as empty (Album Artist only)", GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY,
                         GUID_LASTFM_PREFS_BRANCH_TAG_FALLBACK_COMP, 1.0, false, false, 0); // default OFF

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagTitleRadio0("TITLE", GUID_LASTFM_TAG_TITLE_RADIO_0,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_TITLE, 0.0,
                                                                                true, true, 0); // default ON

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagTitleRadio1("WORK", GUID_LASTFM_TAG_TITLE_RADIO_1, GUID_LASTFM_PREFS_BRANCH_TAG_TITLE, 1.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagTitleRadio2("MOVEMENT",
                                                                                GUID_LASTFM_TAG_TITLE_RADIO_2,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_TITLE, 2.0,
                                                                                false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagTitleRadio3("PART", GUID_LASTFM_TAG_TITLE_RADIO_3, GUID_LASTFM_PREFS_BRANCH_TAG_TITLE, 3.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagTitleRadio4("SUBTITLE",
                                                                                GUID_LASTFM_TAG_TITLE_RADIO_4,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_TITLE, 4.0,
                                                                                false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagAlbumRadio0("ALBUM", GUID_LASTFM_TAG_ALBUM_RADIO_0,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM, 0.0,
                                                                                true, true, 0); // default ON

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumRadio1("RELEASE", GUID_LASTFM_TAG_ALBUM_RADIO_1, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM, 1.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl>
    g_tagAlbumRadio2("WORK", GUID_LASTFM_TAG_ALBUM_RADIO_2, GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM, 2.0, false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagAlbumRadio3("ALBUMTITLE",
                                                                                GUID_LASTFM_TAG_ALBUM_RADIO_3,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM, 3.0,
                                                                                false, true, 0);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagAlbumRadio4("DISCNAME",
                                                                                GUID_LASTFM_TAG_ALBUM_RADIO_4,
                                                                                GUID_LASTFM_PREFS_BRANCH_TAG_ALBUM, 4.0,
                                                                                false, true, 0);

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
                                                                               false, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_dynamicRadio1("Only NP notifications",
                                                                               GUID_LASTFM_PREFS_DYNAMIC_RADIO_1,
                                                                               GUID_LASTFM_PREFS_BRANCH_DYNAMIC, 1.0,
                                                                               false, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_dynamicRadio2("NP & Scrobbling",
                                                                               GUID_LASTFM_PREFS_DYNAMIC_RADIO_2,
                                                                               GUID_LASTFM_PREFS_BRANCH_DYNAMIC, 2.0,
                                                                               true, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagArtistRadio0("ARTIST",
                                                                                 GUID_LASTFM_TAG_ARTIST_RADIO_0,
                                                                                 GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                                                 0.0, true, true, 0u); // default ON

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagArtistRadio1("ALBUM ARTIST",
                                                                                 GUID_LASTFM_TAG_ARTIST_RADIO_1,
                                                                                 GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                                                 1.0, false, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagArtistRadio2("PERFORMER",
                                                                                 GUID_LASTFM_TAG_ARTIST_RADIO_2,
                                                                                 GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                                                 2.0, false, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagArtistRadio3("COMPOSER",
                                                                                 GUID_LASTFM_TAG_ARTIST_RADIO_3,
                                                                                 GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                                                 3.0, false, true, 0u);

static service_factory_single_t<advconfig_entry_checkbox_impl> g_tagArtistRadio4("CONDUCTOR",
                                                                                 GUID_LASTFM_TAG_ARTIST_RADIO_4,
                                                                                 GUID_LASTFM_PREFS_BRANCH_TAG_ARTIST,
                                                                                 4.0, false, true, 0u);

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

static void enforceOneOf5(const GUID& g0, const GUID& g1, const GUID& g2, const GUID& g3, const GUID& g4,
                          std::size_t defaultIndex)
{
    const GUID ids[5] = {g0, g1, g2, g3, g4};
    enforceOneOfN(ids, 5, defaultIndex);
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

static void ensureTagArtistDefaultAdv()
{
    // Default = ARTIST (index 0)
    enforceOneOf5(GUID_LASTFM_TAG_ARTIST_RADIO_0, GUID_LASTFM_TAG_ARTIST_RADIO_1, GUID_LASTFM_TAG_ARTIST_RADIO_2,
                  GUID_LASTFM_TAG_ARTIST_RADIO_3, GUID_LASTFM_TAG_ARTIST_RADIO_4, 0);
}

static int getTagArtistSourceChoice()
{
    ensureTagArtistDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_TAG_ARTIST_RADIO_4))
        return 4;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ARTIST_RADIO_3))
        return 3;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ARTIST_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ARTIST_RADIO_1))
        return 1;
    return 0;
}

static void ensureTagAlbumArtistDefaultAdv()
{
    // Default = ALBUM ARTIST (index 0)
    enforceOneOf5(GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_0, GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_1,
                  GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_2, GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_3,
                  GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_4, 0);
}

static int getTagAlbumArtistSourceChoice()
{
    ensureTagAlbumArtistDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_4))
        return 4;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_3))
        return 3;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_ARTIST_RADIO_1))
        return 1;
    return 0;
}

static void ensureTagTitleDefaultAdv()
{
    // Default = TITLE (index 0)
    enforceOneOf5(GUID_LASTFM_TAG_TITLE_RADIO_0, GUID_LASTFM_TAG_TITLE_RADIO_1, GUID_LASTFM_TAG_TITLE_RADIO_2,
                  GUID_LASTFM_TAG_TITLE_RADIO_3, GUID_LASTFM_TAG_TITLE_RADIO_4, 0);
}

static int getTagTitleSourceChoice()
{
    ensureTagTitleDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_TAG_TITLE_RADIO_4))
        return 4;
    if (advGetCheckboxState(GUID_LASTFM_TAG_TITLE_RADIO_3))
        return 3;
    if (advGetCheckboxState(GUID_LASTFM_TAG_TITLE_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_TAG_TITLE_RADIO_1))
        return 1;
    return 0;
}

static void ensureTagAlbumDefaultAdv()
{
    // Default = ALBUM (index 0)
    enforceOneOf5(GUID_LASTFM_TAG_ALBUM_RADIO_0, GUID_LASTFM_TAG_ALBUM_RADIO_1, GUID_LASTFM_TAG_ALBUM_RADIO_2,
                  GUID_LASTFM_TAG_ALBUM_RADIO_3, GUID_LASTFM_TAG_ALBUM_RADIO_4, 0);
}

static int getTagAlbumSourceChoice()
{
    ensureTagAlbumDefaultAdv();

    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_RADIO_4))
        return 4;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_RADIO_3))
        return 3;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_RADIO_2))
        return 2;
    if (advGetCheckboxState(GUID_LASTFM_TAG_ALBUM_RADIO_1))
        return 1;
    return 0;
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
{ return advGetCheckboxState(GUID_LASTFM_PREFS_CHECKBOX_1); }

int lastfmDynamicSourcesMode()
{ return getDynamicSourcesMode(); }

bool lastfmDisableNowPlaying()
{ return advGetCheckboxState(GUID_LASTFM_PREFS_CHECKBOX_0); }

int lastfmTagArtistSource()
{ return getTagArtistSourceChoice(); }

bool lastfmTagFallbackArtistAlbum()
{ return advGetCheckboxState(GUID_LASTFM_TAG_CHECKBOX_FALLBACK_ARTIST_ALBUM); }

bool lastfmTagTreatVariousArtistsAsEmpty()
{ return advGetCheckboxState(GUID_LASTFM_TAG_CHECKBOX_VA_AS_EMPTY); }

int lastfmTagAlbumArtistSource()
{ return getTagAlbumArtistSourceChoice(); }

int lastfmTagTitleSource()
{ return getTagTitleSourceChoice(); }

int lastfmTagAlbumSource()
{ return getTagAlbumSourceChoice(); }
