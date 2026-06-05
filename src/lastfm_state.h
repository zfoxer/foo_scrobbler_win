//
//  lastfm_state.h
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#pragma once

#include <foobar2000/SDK/foobar2000.h>

#include <string>

// cfg-backed state (no UI, no dialogs)

struct LastfmAuthState
{
    bool isAuthenticated = false;
    bool isSuspended = false;
    std::string username;   // Last.fm username
    std::string sessionKey; // 'sk' from Last.fm
};

// Returns the current cached auth state (from config)
LastfmAuthState lastfmGetAuthState();

// Used by the auth layer to persist successful auth.
void lastfmSetAuthState(const LastfmAuthState& state);

bool lastfmIsAuthenticated();
bool lastfmIsSuspended();

// Mutators
void lastfmClearAuthentication();
void lastfmClearSuspension();
void lastfmSuspendCurrentUser();

// Queue ownership (prevents cross-account submission)
pfc::string8 lastfmGetQueueOwnerUsername();
void lastfmSetQueueOwnerUsername(const char* username);
void lastfmClearQueueOwnerUsername();
