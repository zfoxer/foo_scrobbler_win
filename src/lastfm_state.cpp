//
//  lastfm_state.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"
#include "lastfm_state.h"
#include "debug.h"

#include <foobar2000/SDK/foobar2000.h>

#include <mutex>

static const GUID GUID_CFG_LASTFM_AUTHENTICATED = { 
    0xc4eeff3c, 0x4f04, 0x4a89, {0xa4, 0x3e, 0x16, 0x31, 0x86, 0x1b, 0xe6, 0x27} };

static const GUID GUID_CFG_LASTFM_USERNAME = { 
    0x180daa7f, 0x6eb3, 0x4735, {0xa3, 0xb0, 0xf0, 0x62, 0x6a, 0x57, 0x31, 0x13} };

static const GUID GUID_CFG_LASTFM_SESSION_KEY = { 
    0x31947070, 0xf42e, 0x4417, {0x89, 0xa8, 0x12, 0x31, 0xad, 0xde, 0x89, 0x68} };

static const GUID GUID_CFG_LASTFM_SUSPENDED = { 
    0xc44cbaca, 0xa412, 0x4cd7, {0x8e, 0xb4, 0xac, 0xa9, 0x7a, 0x5e, 0x1c, 0x95} };

static const GUID GUID_CFG_LASTFM_QUEUE_OWNER_USERNAME = { 
    0x26c28ecf, 0x5992, 0x4737, {0x81, 0xf0, 0x20, 0x4c, 0x23, 0xc8, 0x3f, 0xb8} };

static cfg_string cfgLastfmQueueOwner(GUID_CFG_LASTFM_QUEUE_OWNER_USERNAME, "");
static cfg_bool cfgLastfmAuthenticated(GUID_CFG_LASTFM_AUTHENTICATED, false);
static cfg_string cfgLastfmUsername(GUID_CFG_LASTFM_USERNAME, "");
static cfg_string cfgLastfmSessionKey(GUID_CFG_LASTFM_SESSION_KEY, "");
static cfg_bool cfgLastfmSuspended(GUID_CFG_LASTFM_SUSPENDED, false);

static std::mutex authMutex;

LastfmAuthState lastfmGetAuthState()
{
    std::lock_guard<std::mutex> lock(authMutex);
    LastfmAuthState state;
    state.isAuthenticated = cfgLastfmAuthenticated.get();
    state.username = cfgLastfmUsername.get();
    state.sessionKey = cfgLastfmSessionKey.get();
    state.isSuspended = cfgLastfmSuspended.get();
    return state;
}

void lastfmSetAuthState(const LastfmAuthState& state)
{
    std::lock_guard<std::mutex> lock(authMutex);
    cfgLastfmAuthenticated = state.isAuthenticated;
    cfgLastfmUsername.set(state.username.c_str());
    cfgLastfmSessionKey.set(state.sessionKey.c_str());
    if (state.isAuthenticated)
        cfgLastfmSuspended = false;
}

bool lastfmIsAuthenticated()
{
    std::lock_guard<std::mutex> lock(authMutex);
    return cfgLastfmAuthenticated.get();
}

bool lastfmIsSuspended()
{
    std::lock_guard<std::mutex> lock(authMutex);
    return cfgLastfmSuspended.get();
}

void lastfmClearAuthentication()
{
    LFM_DEBUG("Clearing cfg state.");

    pfc::string8 user;
    {
        std::lock_guard<std::mutex> lock(authMutex);
        user = cfgLastfmUsername.get();
        cfgLastfmAuthenticated = false;
        cfgLastfmUsername.set("");
        cfgLastfmSessionKey.set("");
        cfgLastfmSuspended = false;
    }

    pfc::string_formatter f;
    f << "Now authenticated=" << (lastfmIsAuthenticated() ? 1 : 0) << ", user='" << user << "'";
    LFM_INFO(f.c_str());
}

void lastfmClearSuspension()
{
    LFM_DEBUG("Clearing suspend state.");

    pfc::string8 user;
    {
        std::lock_guard<std::mutex> lock(authMutex);
        cfgLastfmSuspended = false;
        user = cfgLastfmUsername.get();
    }

    pfc::string_formatter f;
    f << "Suspended=" << (lastfmIsSuspended() ? "yes" : "no") << ", user='" << user << "'";
    LFM_INFO(f.c_str());
}

void lastfmSuspendCurrentUser()
{
    LFM_DEBUG("Suspending current user.");

    pfc::string8 user;
    {
        std::lock_guard<std::mutex> lock(authMutex);
        cfgLastfmSuspended = true;
        user = cfgLastfmUsername.get();
    }

    pfc::string_formatter f;
    f << "Suspended=" << (lastfmIsSuspended() ? "yes" : "no") << ", user='" << user << "'";
    LFM_INFO(f.c_str());
}

pfc::string8 lastfmGetQueueOwnerUsername()
{
    std::lock_guard<std::mutex> lock(authMutex);
    return cfgLastfmQueueOwner.get();
}

void lastfmSetQueueOwnerUsername(const char* username)
{
    std::lock_guard<std::mutex> lock(authMutex);
    cfgLastfmQueueOwner.set(username ? username : "");
}

void lastfmClearQueueOwnerUsername()
{
    std::lock_guard<std::mutex> lock(authMutex);
    cfgLastfmQueueOwner.set("");
}
