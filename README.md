### Foo Scrobbler for Windows
#### Version: 1.0.0 — foo_scrobbler_win — Released under GNU GPLv3
#### © 2025-2026 by Konstantinos Kyriakopoulos

#### See the detailed [Installation Guide](https://github.com/zfoxer/foo_scrobbler_mac/wiki/Installation) and [Last.fm Authentication Guide](https://github.com/zfoxer/foo_scrobbler_mac/wiki/LFM_Auth).

### Intro

Foo Scrobbler (foo_scrobbler_win) is a native Last.fm scrobbling plugin for foobar2000 on Windows. Submits tracks based on precise playback rules, caches scrobbles when offline, and operates silently after one-time authentication. Built using the official foobar2000 plugin API, it focuses on reliability, low overhead, and correct metadata handling. Fully open-source under GPLv3.

Supports Windows 10 (x86, x64) and Windows 11 (x64).  


### Key Features

- **Native Windows Last.fm scrobbling**  
  Fully integrated with foobar2000 for Windows. No compatibility layers or wrapper apps.

- **Rule-based submission logic**  
  Scrobbles only when playback is meaningful (e.g., ≥ 50% or ≥ 240 seconds).

- **Automatic offline caching**  
  If Last.fm or the network is unavailable, scrobbles are stored and submitted automatically later.

- **Accurate “Now Playing” handling**  
  Fully aligned with Last.fm Scrobbling 2.0 API specifications.

- **Minimal user interaction**  
  Authentication required only once.

- **Lightweight and efficient**  
  Runs inside foobar2000 without performance loss. Not relying on third-party dependencies.

- **Strict playback validation**  
  Prevents malformed or duplicate scrobbles.

- **Open-source (GPLv3)**  
  Transparent and extensible.


### Usage

Install **foo_scrobbler_win.fb2k-component** from within foobar2000 right from the components section in preferences.  

Authentication requires only an active Last.fm account. Users grant access once through the Last.fm website with their account, after which Foo Scrobbler runs quietly in the background and submits track information automatically. If authentication is cleared from the menu, the same user —or a different one— must grant access again through browser redirection to the Last.fm website. Foo Scrobbler adds a simple, convenient and non-intrusive last entry under Playback in the menu bar.  More options are located in Preferences → Advanced → Tools → Foo Scrobbler.


### Licensing Notice

This project is licensed under the GNU GPLv3.

The SDK is proprietary and **not covered by the GPL license**. It remains the property of its original author (Peter Pawlowski / foobar2000).

Only the source code of the Foo Scrobbler plugin is licensed under GPLv3.

### Changelog

<pre>

1.0.0    2026-02-XX    Initial Windows release. Sharing codebase with the macOS version.
</pre>

