<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/foo-scrobbler_dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="assets/foo-scrobbler_light.svg">
    <img alt="Foo Scrobbler" src="assets/foo-scrobbler_light.svg" width="420">
  </picture>
</p>

### Foo Scrobbler for Windows    

**Release:** 1.1.1  
**License:** MIT  
**Copyright:** © 2025–2026 Konstantinos Kyriakopoulos  

A native Lastfm scrobbler component (foo_scrobbler_win) for foobar2000 on Windows. It submits “Now Playing” and scrobbles using the official Last.fm Scrobbling 2.0 API, applies strict playback qualification rules, and keeps a local queue when you’re offline. Once authenticated, it runs quietly in the background.


**OS support:** Windows 10 (x86, x64) and Windows 11 (x64, arm64ec) for foobar2000 ≥ v2.24

This is the GitHub site of the [Windows version](https://github.com/zfoxer/foo_scrobbler_win).  
For the macOS version of Foo Scrobbler [see here](https://github.com/zfoxer/foo_scrobbler_mac).

### Quick start

1. In foobar, go to **Preferences → Components**.
2. Install: **foo_scrobbler_win.fb2k-component**.
3. [Authenticate](https://github.com/zfoxer/foo_scrobbler_win/wiki/LFM_Auth) once with your Lastfm account through the browse flow.  
4. Play music. Scrobbling happens automatically.

### Where the UI lives
- Main menu: **Playback → Last.fm**
- Settings: **File → Preferences → Advanced → Tools → Foo Scrobbler**


### What it does

#### Submission behavior
- Sends **Now Playing** when appropriate (aligned with Last.fm Scrobbling 2.0 expectations).
- Scrobbles only after playback qualifies (e.g., **50% played** or **240 seconds**, whichever comes first).
- Uses validation to prevent malformed or duplicate submissions.

#### When the network is unreliable
- If Last.fm can’t be reached, scrobbles are **queued locally**.
- When connectivity returns, the queue is **flushed automatically**.


### Design goals and features

- **Native component**: Runs inside foobar2000 on Windows, no wrappers.  
- **Predictable rules**: Deterministic scrobble qualification.  
- **Offline caching**: Stores failed scrobbles and submits them later.  
- **Now Playing**: Handles Last.fm Now Playing updates correctly.  
- **One-time auth**: Requires authentication only once.  
- **Low overhead**: Lean implementation with no third-party dependencies.  
- **Playback validation**: Rejects malformed, invalid, or duplicate scrobbles.  
- **Dynamic sources**: Fully supports radio streams and dynamic playback metadata.  
- **Title Formatting**: Uses foobar2000 Title Formatting for input metadata.  
- **Regex filtering**: Filters unwanted scrobbles using regular expressions.  
- **Correct metadata handling**: Preserves valid tag values before submission.  
- **Console logging**: Fully reports impactful internal scrobbling actions.  
- **Open source**: Released under the MIT License.  

### Documentation

- Technical description: https://github.com/zfoxer/foo_scrobbler_mac/wiki  

<p align="left">
  <img src="assets/prefs_win.png" alt="Foo Scrobbler Settings" width="800" />
</p>


### Release notes

<details>
<summary><strong>Show changelog</strong></summary>

<pre>
1.1.1    2026-05-11    Preserve order of TF input and TF exclusion filters.
                       Improve batch scrobbling error logs.
                       Use backed-off, not-due scrobbles, to fill batches for small queues.

1.1.0    2026-05-09    Update queue draining with API-supported batch scrobbling.
                       Fix rare worker races that could cause busy spins and stale queue retries.
                       Add exclusion filtering using foobar2000 Title Formatting.
                       Add regex filtering support for albums.
                       Ignore scrobbling and Now Playing according to the FOO_SCROBBLER tag flag.
                       Replace Windows BCrypt MD5 hashing with the SDK-native implementation.

1.0.9    2026-03-26    Avoid reparsing the persisted scrobble queue on every access.
                       Cache compiled titleformat scripts instead of rebuilding them during playback.
                       Replace unsafe static locals in stream dedup with per-instance tracker state.
                       Last.fm back-end error 8 treated as temp, not having limited retries before discarding.
                       Fix regression for URL opener (Windows-only).

1.0.7    2026-03-20    Handle Last.fm rate-limit error 29 with queue cooldown.
                       Added MUSICBRAINZ_TRACKID in scrobbling and NP dispatch data.
                       Merged NP code into WebAPI.
                       Fix UI dynamic sources setting being disconnected (Windows-only).

1.0.6    2026-03-13    Added support for foobar Title Formatting for input tags. Removed previous tag mapping.
                       Added build for ARM64EC architecture for Windows 11.

1.0.5    2026-03-07    Added regular expressions (regex) support to filter out submissions (Advanced prefs).
                       Fixed: Unicode track titles are now handled correctly for Now Playing and scrobbling.
                       Switched to MIT License

1.0.2    2026-03-01    Initial Windows release. Sharing codebase with the macOS version.
</pre>

</details>
