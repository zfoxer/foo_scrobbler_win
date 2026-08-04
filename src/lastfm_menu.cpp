//
//  lastfm_menu.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "lastfm_menu.h"
#include "lastfm_core.h"
#include "lastfm_state.h"
#include "lastfm_settings.h"
#include "lastfm_util.h"
#include "debug.h"

#include <foobar2000/SDK/foobar2000.h>
#include <foobar2000/SDK/threadPool.h>

#include <atomic>
#include <string>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

static const GUID GUID_LASTFM_AUTHENTICATE = {
    0x505126a6, 0xcd87, 0x47bb, {0xaf, 0xf3, 0x45, 0x90, 0xfa, 0xed, 0xe8, 0x01}};

static const GUID GUID_LASTFM_CLEAR_AUTH = {
    0x93df02d7, 0x6ed9, 0x4633, {0xa5, 0x03, 0x1e, 0xe2, 0x60, 0x12, 0x1c, 0xaa}};

static const GUID GUID_LASTFM_MENU_GROUP = {
    0x9dd92f54, 0xbb91, 0x49a1, {0xaf, 0x09, 0x5f, 0x23, 0x9c, 0x7d, 0x17, 0x8f}};

static const GUID GUID_LASTFM_SUSPEND = {0x7e72e458, 0x3ac9, 0x4942, {0xad, 0xc3, 0x69, 0xf1, 0x13, 0xdb, 0x38, 0xef}};

namespace
{

static bool playbackMenuVisible()
{
    return lastfm::settings::showPlaybackMenu() || !lastfmIsAuthenticated();
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

static void openBrowserUrl(const std::string& url)
{
#if defined(__APPLE__)
    if (url.find('"') != std::string::npos)
        return;
    std::string cmd = "open \"" + url + "\"";
    std::system(cmd.c_str());
#elif defined(_WIN32)
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
#else
    LFM_INFO("Open manually: (url omitted)");
#endif
}

static std::atomic<bool> authRequestInFlight{false};

static void runAuthenticateFlow()
{
    auto& authenticator = LastfmCore::instance().authenticator();

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

            lastfmSetAuthState(state);
            popup_message::g_show("Authentication complete.", "Foo Scrobbler");

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
                popup_message::g_show("Authorization was not completed. Let's try again.\n"
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
    if (!playbackMenuVisible())
        return false;

    const bool authed = lastfmIsAuthenticated();

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
    switch (index)
    {
    case CMD_AUTHENTICATE:
    {
        if (lastfmIsAuthenticated())
            return;

        if (authRequestInFlight.exchange(true))
        {
            LFM_DEBUG("Authentication request already in progress.");
            return;
        }

        fb2k::inWorkerThread(
            []
            {
                runAuthenticateFlow();
                authRequestInFlight.store(false);
            });
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
