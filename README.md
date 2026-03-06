# Foo Scrobbler for Windows    

**Release:** 1.0.5  
**License:** MIT  
**Copyright:** © 2025–2026 Konstantinos Kyriakopoulos  

A native Lastfm scrobbler component (foo_scrobbler_win) for foobar2000 on Windows. It submits “Now Playing” and scrobbles using the official Last.fm Scrobbling 2.0 API, applies strict playback qualification rules, and keeps a local queue when you’re offline. Once authenticated, it runs quietly in the background.


**OS support:** Windows 10 (x86, x64) and Windows 11 (x64)

This is the GitHub site of the [Windows version](https://github.com/zfoxer/foo_scrobbler_win).  
For the macOS version of Foo Scrobbler [see here](https://github.com/zfoxer/foo_scrobbler_mac).

## Quick start

1. In foobar, go to **Preferences → Components**.
2. Install: **foo_scrobbler_win.fb2k-component**.
3. [Authenticate](https://github.com/zfoxer/foo_scrobbler_win/wiki/LFM_Auth) once with your Lastfm account through the browse flow.  
4. Play music. Scrobbling happens automatically.

**Where the UI lives**
- Main menu: **Playback → Last.fm**
- Settings: **File → Preferences → Advanced → Tools → Foo Scrobbler**


## What it does

### Submission behavior
- Sends **Now Playing** when appropriate (aligned with Last.fm Scrobbling 2.0 expectations).
- Scrobbles only after playback qualifies (e.g., **50% played** or **240 seconds**, whichever comes first).
- Uses validation to prevent malformed or duplicate submissions.

### When the network is unreliable
- If Last.fm can’t be reached, scrobbles are **queued locally**.
- When connectivity returns, the queue is **flushed automatically**.


## Design goals

- **Native component**: Runs inside foobar2000 on Windows, no wrappers.
- **Predictable rules**: Deterministic scrobble qualification.
- **Low overhead**: Lean implementation with no third-party dependencies.
- **Correct metadata handling**: Treats tags as input, not something to “fix”.


## Documentation

- Technical description: https://github.com/zfoxer/foo_scrobbler_mac/wiki  


## Licensing notes

This repository’s **Foo Scrobbler plugin source code** is licensed under **MIT License**.

The **foobar2000 SDK** is proprietary and is **not** covered by the MIT License. It remains the property of its original author (Peter Pawlowski / foobar2000).


## Release notes

<details>
<summary><strong>Show changelog</strong></summary>

<pre>
1.0.5    2026-03-07    Added regular expressions (regex) support to filter out submissions (Advanced prefs).
                       Switched to MIT License

1.0.2    2026-03-01    Initial Windows release. Sharing codebase with the macOS version.
</pre>

</details>
