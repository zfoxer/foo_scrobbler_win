//
//  lastfm_menu.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"
#include "lastfm_menu.h"
#include "lastfm_ui.h"
#include "lastfm_core.h"
#include "lastfm_track_info.h"
#include "lastfm_state.h"
#include "debug.h"

#include <foobar2000/SDK/foobar2000.h>

#include <string>
#include <cstdlib>
#include <cctype>

static const GUID GUID_LASTFM_AUTHENTICATE = {
    0x505126a6, 0xcd87, 0x47bb, {0xaf, 0xf3, 0x45, 0x90, 0xfa, 0xed, 0xe8, 0x01}};

static const GUID GUID_LASTFM_CLEAR_AUTH = {
    0x93df02d7, 0x6ed9, 0x4633, {0xa5, 0x03, 0x1e, 0xe2, 0x60, 0x12, 0x1c, 0xaa}};

static const GUID GUID_LASTFM_MENU_GROUP = {
    0x9dd92f54, 0xbb91, 0x49a1, {0xaf, 0x09, 0x5f, 0x23, 0x9c, 0x7d, 0x17, 0x8f}};

static const GUID GUID_LASTFM_SUSPEND = {0x7e72e458, 0x3ac9, 0x4942, {0xad, 0xc3, 0x69, 0xf1, 0x13, 0xdb, 0x38, 0xef}};

static mainmenu_group_popup_factory lastfmMenuGroupFactory(GUID_LASTFM_MENU_GROUP, mainmenu_groups::playback,
                                                           mainmenu_commands::sort_priority_dontcare, "Last.fm");

namespace
{

static void openBrowserUrl(const std::string& url)
{
#if defined(__APPLE__)
    if (url.find('"') != std::string::npos)
        return;
    std::string cmd = "open \"" + url + "\"";
    std::system(cmd.c_str());
#else
    LFM_INFO("Open manually: (url omitted)");
#endif
}

static std::string cleanTagValue(const char* value)
{
    if (!value)
        return {};

    std::string s(value);

    std::size_t start = 0;
    while (start < s.size() && std::isspace((unsigned char)s[start]))
        ++start;

    std::size_t end = s.size();
    while (end > start && std::isspace((unsigned char)s[end - 1]))
        --end;

    if (start == end)
        return {};

    s = s.substr(start, end - start);

    std::string norm;
    for (char c : s)
        if (!std::isspace((unsigned char)c))
            norm.push_back((char)std::tolower((unsigned char)c));

    if (norm == "unknown" || norm == "unknownartist" || norm == "unknowntrack")
        return {};

    return s;
}

static bool getNowPlayingTrackInfo(LastfmTrackInfo& out)
{
    out = LastfmTrackInfo{};

    metadb_handle_ptr handle;
    if (!playback_control::get()->get_now_playing(handle) || !handle.is_valid())
        return false;

    file_info_impl info;
    if (!handle->get_info(info))
        return false;

    out.artist = cleanTagValue(info.meta_get("artist", 0));
    out.title = cleanTagValue(info.meta_get("title", 0));
    out.album = cleanTagValue(info.meta_get("album", 0));

    const char* mbid = info.meta_get("musicbrainz_trackid", 0);
    if (!mbid)
        mbid = info.meta_get("MUSICBRAINZ_TRACKID", 0);
    out.mbid = mbid ? mbid : "";

    out.durationSeconds = info.get_length();

    // For Now Playing, if artist/title are missing, just skip sending.
    return !out.artist.empty() && !out.title.empty();
}
} // namespace

t_uint32 LastfmMenu::get_command_count()
{
    return CMD_COUNT;
}

GUID LastfmMenu::get_command(t_uint32 index)
{
    switch (index)
    {
    case CMD_AUTHENTICATE:
        return GUID_LASTFM_AUTHENTICATE;
    case CMD_CLEAR_AUTH:
        return GUID_LASTFM_CLEAR_AUTH;
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
    case CMD_AUTHENTICATE:
        out = "Authenticate";
        break;
    case CMD_CLEAR_AUTH:
        out = "Clear authentication";
        break;
    case CMD_SUSPEND:
        out = isSuspended() ? "Resume scrobbling" : "Pause scrobbling";
        break;
    default:
        uBugCheck();
    }
}

bool LastfmMenu::get_description(t_uint32 index, pfc::string_base& out)
{
    switch (index)
    {
    case CMD_AUTHENTICATE:
        out = "Authenticate this foobar2000 instance with Last.fm.";
        return true;
    case CMD_CLEAR_AUTH:
        out = "Clear stored Last.fm authentication/session key.";
        return true;
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
    const bool authed = isAuthenticated();

    switch (index)
    {
    case CMD_AUTHENTICATE:
        if (authed)
            return false;
        break;
    case CMD_CLEAR_AUTH:
    case CMD_SUSPEND:
        if (!authed)
            return false;
        break;
    default:
        return false;
    }

    get_name(index, text);
    return true;
}

void LastfmMenu::execute(t_uint32 index, ctx_t)
{
    auto& authenticator = LastfmCore::instance().authenticator();
    switch (index)
    {
    case CMD_AUTHENTICATE:
    {
        if (isAuthenticated())
            return;

        std::string url;

        if (!authenticator.hasPendingToken())
        {
            const bool ok = authenticator.startAuth(url);
            if (ok && !url.empty())
            {
                popup_message::g_show("A browser window will open to authorize this foobar2000 instance with Last.fm.\n"
                                      "After allowing access, return here and click Authenticate again.",
                                      "Foo Scrobbler");
                openBrowserUrl(url);
            }
            else
            {
                authenticator.logout(); // Clear any half-started state
                popup_message::g_show("Failed to start authentication. Please try again.", "Foo Scrobbler");
            }
        }
        else
        {
            LastfmAuthState state;
            if (authenticator.completeAuth(state))
            {
                popup_message::g_show("Authentication complete.", "Foo Scrobbler");

                auto& core = LastfmCore::instance();

                // Prevent cross-account submission:
                const pfc::string8 owner = lastfmGetQueueOwnerUsername();
                const std::string newUser = state.username;

                if (owner.is_empty())
                {
                    // First time: claim ownership.
                    lastfmSetQueueOwnerUsername(newUser.c_str());
                }
                else if (std::string(owner.c_str()) != newUser)
                {
                    // Different user: wipe pending scrobbles before draining.
                    core.scrobbler().clearQueue();
                    lastfmSetQueueOwnerUsername(newUser.c_str());
                }
                // else same user -> keep queue as-is
                core.scrobbler().onAuthenticationRecovered();
                core.scrobbler().retryAsync();
            }
            else
            {
                // User likely closed browser or denied access. Reset and restart auth flow.
                authenticator.logout();

                std::string url2;
                if (authenticator.startAuth(url2) && !url2.empty())
                {
                    popup_message::g_show(
                        "Authorization was not completed. Let's try again.\n"
                        "A browser window will open to authorize this foobar2000 instance with Last.fm.\n"
                        "After allowing access, return here and click Authenticate again.",
                        "Foo Scrobbler");
                    openBrowserUrl(url2);
                }
                else
                {
                    popup_message::g_show("Authentication failed. Please try again.", "Foo Scrobbler");
                }
            }
        }
        break;
    }

    case CMD_CLEAR_AUTH:
    {
        auto& core = LastfmCore::instance();

        // Do NOT clear the queue here anymore.
        // Do NOT clear queue-owner either.
        core.scrobbler().resetInvalidSessionHandling();
        core.authenticator().logout();

        popup_message::g_show("Stored Last.fm authentication has been cleared.\n"
                              "Pending scrobbles are kept for this user.",
                              "Foo Scrobbler");
        break;
    }

    case CMD_SUSPEND:
    {
        auto& core = LastfmCore::instance();

        if (isSuspended())
        {
            clearSuspension();

            // Send Now Playing immediately for the currently playing track.
            // Use sendNowPlayingOnly() so we do NOT flush the retry queue on resume.
            LastfmTrackInfo now;
            if (getNowPlayingTrackInfo(now))
                core.scrobbler().sendNowPlayingOnly(now);

            // Do NOT retryAsync() here.
            // Queue will be retried on natural boundaries (next track -> onNowPlaying, stop -> retryAsync, etc).
        }
        else
        {
            suspendCurrentUser();
        }
        break;
    }

    default:
        uBugCheck();
    }
}

static mainmenu_commands_factory_t<LastfmMenu> lastfmMenuFactory;
