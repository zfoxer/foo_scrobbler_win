//
//  lastfm_menu.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "lastfm_menu.h"
#include "lastfm_state.h"
#include "lastfm_settings.h"

#include <foobar2000/SDK/foobar2000.h>

static const GUID GUID_LASTFM_MENU_GROUP = {
    0x9dd92f54, 0xbb91, 0x49a1, {0xaf, 0x09, 0x5f, 0x23, 0x9c, 0x7d, 0x17, 0x8f}};

static const GUID GUID_LASTFM_SUSPEND = {0x7e72e458, 0x3ac9, 0x4942, {0xad, 0xc3, 0x69, 0xf1, 0x13, 0xdb, 0x38, 0xef}};

namespace
{

static bool playbackMenuVisible()
{
    return lastfm::settings::showPlaybackMenu() && lastfmIsAuthenticated();
}

class LastfmMenuGroup : public mainmenu_group_popup_v2
{
  public:
    GUID get_guid() override
    {
        return GUID_LASTFM_MENU_GROUP;
    }
    GUID get_parent() override
    {
        return mainmenu_groups::playback;
    }
    t_uint32 get_sort_priority() override
    {
        return mainmenu_commands::sort_priority_dontcare;
    }
    void get_display_string(pfc::string_base& out) override
    {
        out = "Last.fm";
    }
    bool popup_condition() override
    {
        return playbackMenuVisible();
    }
};

FB2K_SERVICE_FACTORY(LastfmMenuGroup);
} // namespace

t_uint32 LastfmMenu::get_command_count()
{
    return CMD_COUNT;
}

GUID LastfmMenu::get_command(t_uint32 index)
{
    switch (index)
    {
    case CMD_SUSPEND:
        return GUID_LASTFM_SUSPEND;
    default:
        uBugCheck();
    }
}

void LastfmMenu::get_name(t_uint32 index, pfc::string_base& out)
{
    switch (index)
    {
    case CMD_SUSPEND:
        out = lastfmIsSuspended() ? "Resume scrobbling" : "Pause scrobbling";
        break;
    default:
        uBugCheck();
    }
}

bool LastfmMenu::get_description(t_uint32 index, pfc::string_base& out)
{
    switch (index)
    {
    case CMD_SUSPEND:
        out = "Suspend user from scrobbling.";
        return true;
    default:
        return false;
    }
}

GUID LastfmMenu::get_parent()
{
    return GUID_LASTFM_MENU_GROUP;
}

t_uint32 LastfmMenu::get_sort_priority()
{
    return sort_priority_dontcare;
}

bool LastfmMenu::get_display(t_uint32 index, pfc::string_base& text, uint32_t& flags)
{
    flags = 0;
    if (index != CMD_SUSPEND || !playbackMenuVisible())
        return false;

    get_name(index, text);
    return true;
}

void LastfmMenu::execute(t_uint32 index, ctx_t)
{
    switch (index)
    {
    case CMD_SUSPEND:
    {
        if (lastfmIsSuspended())
        {
            lastfmClearSuspension();

            // The tracker notices the resume on its next playback tick and re-sends
            // Now Playing for the current track (NP-only path).

            // Do NOT retryAsync() here.
            // Queue will be retried on natural boundaries (next track -> onNowPlaying, stop -> retryAsync, etc).
        }
        else
        {
            lastfmSuspendCurrentUser();
        }
        break;
    }

    default:
        uBugCheck();
    }
}

static mainmenu_commands_factory_t<LastfmMenu> lastfmMenuFactory;
