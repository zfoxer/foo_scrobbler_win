//
//  lastfm_ui.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"
#include "lastfm_ui.h"
#include "lastfm_state.h"
#include "lastfm_prefs_pane_state.h"

#include <foobar2000/SDK/advconfig.h>
#include <foobar2000/SDK/foobar2000.h>

// Reuse the exact GUID from lastfm_prefs_pane.cpp:
static const GUID GUID_LASTFM_PREFS_CHECKBOX_0 = {
    0x9377030d, 0xc480, 0x4322, {0xa7, 0x55, 0x63, 0xfa, 0x5c, 0xa0, 0x03, 0xb0}};

bool lastfmDisableNowplaying()
{
    service_ptr_t<advconfig_entry_checkbox> e;
    if (!advconfig_entry::g_find_t(e, GUID_LASTFM_PREFS_CHECKBOX_0))
        return false; // default = enabled
    return e->get_state();
}

LastfmAuthState getAuthState()
{
    return lastfmGetAuthState();
}

void setAuthState(const LastfmAuthState& state)
{
    lastfmSetAuthState(state);
}

bool isAuthenticated()
{
    return lastfmIsAuthenticated();
}

void clearAuthentication()
{
    lastfmClearAuthentication();
}

void clearSuspension()
{
    lastfmClearSuspension();
}

bool isSuspended()
{
    return lastfmIsSuspended();
}

void suspendCurrentUser()
{
    lastfmSuspendCurrentUser();
}

int getPrefsPaneRadioChoice()
{
    return lastfmGetPrefsPaneRadioChoice();
}

void setPrefsPaneRadioChoice(int value)
{
    lastfmSetPrefsPaneRadioChoice(value);
}

bool getPrefsPaneCheckbox()
{
    return lastfmGetPrefsPaneCheckbox();
}

void setPrefsPaneCheckbox(bool enabled)
{
    lastfmSetPrefsPaneCheckbox(enabled);
}
