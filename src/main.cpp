//
//  main.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"
#include "sdk_bootstrap.h"
#include <foobar2000/SDK/foobar2000.h>

#include "version.h"
#include "debug.h"
#include "lastfm_core.h"

// Component GUID
static const GUID FOO_SCROBBLER_WIN_GUID = { 
    0xc38c2b70, 0x0edb, 0x432a, {0x81, 0xac, 0x15, 0x2f, 0x1e, 0x94, 0x5d, 0x96} };

// Component version info
DECLARE_COMPONENT_VERSION("Foo Scrobbler", FOOSCROBBLER_VERSION,
                          "A Last.fm scrobbler for foobar2000 (Windows).\n"
                          "(c) 2025-2026 Konstantinos Kyriakopoulos.\n"
                          "GPLv3-licensed source.");

// Ensures the binary filename is correct
VALIDATE_COMPONENT_FILENAME("foo_scrobbler_win.dll");

// Init/quit handler
class FooScrobblerWinComponent : public initquit
{
  public:
    void on_init() override
    {
        console::formatter f;
        f << FOOSCROBBLER_NAME << " " << FOOSCROBBLER_VERSION;
    }

    void on_quit() override
    {
        LastfmCore::instance().scrobbler().shutdown();
    }
};

static initquit_factory_t<FooScrobblerWinComponent> initquitFactory;
