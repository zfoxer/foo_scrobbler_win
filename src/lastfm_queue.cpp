//
//  lastfm_queue.cpp
//  foo_scrobbler_win
//
//  (c) 2025-2026 by Konstantinos Kyriakopoulos
//

#include "stdafx.h"

#include "lastfm_queue.h"
#include "lastfm_client.h"
#include "debug.h"

#include <foobar2000/SDK/foobar2000.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <vector>
#include <cstdint>
#include <random>
#include <cctype>
#include <cerrno>
#include <atomic>

namespace
{
static const GUID GUID_CFG_LASTFM_PENDING_SCROBBLES = {
    0xc036ebfb, 0xbe8b, 0x4aa1, {0x8c, 0x72, 0x43, 0x31, 0x33, 0xc8, 0x7f, 0xf9}};

static const GUID GUID_CFG_LASTFM_DRAIN_COOLDOWN_SECS = {
    0xcf46798b, 0x6011, 0x493a, {0xbd, 0x69, 0xee, 0x86, 0x65, 0x2b, 0x98, 0x46}};

static const GUID GUID_CFG_LASTFM_DRAIN_ENABLED = {
    0x7d4ab482, 0x72ac, 0x4cc1, {0x8f, 0x5f, 0xd5, 0xc9, 0xf6, 0xd3, 0xc2, 0x61}};

static const GUID GUID_CFG_LASTFM_DAILY_BUDGET = {
    0x63ba0191, 0x2700, 0x43c9, {0x86, 0x24, 0xbc, 0xec, 0xd0, 0x35, 0x66, 0x34}};

static const GUID GUID_CFG_LASTFM_SCROBBLES_TODAY = {
    0x2f909abc, 0x5b2c, 0x480a, {0xa5, 0x65, 0x00, 0x8a, 0x99, 0x0a, 0x91, 0xbb}};

static const GUID GUID_CFG_LASTFM_DAY_STAMP = {
    0xd1d5509e, 0xbb26, 0x451a, {0x89, 0x56, 0x89, 0x32, 0x4e, 0x12, 0x49, 0xb7}};

// Dispatch at most 10 per run
static constexpr size_t K_MAX_DISPATCH_BATCH = 10;

// Linear backoff: 60s, 120s, 180s… capped
static constexpr int K_RETRY_STEP_SECONDS = 60;
static constexpr int K_RETRY_MAX_SECONDS = 60 * 60; // 1h cap
static constexpr int K_RATE_LIMIT_COOLDOWN_SECONDS = 6 * 60;

static cfg_string cfgLastfmPendingScrobbles(GUID_CFG_LASTFM_PENDING_SCROBBLES, "");

static cfg_int cfgLastfmDrainCooldownSeconds(GUID_CFG_LASTFM_DRAIN_COOLDOWN_SECS,
                                             360 // 6 minutes
);

static cfg_int cfgLastfmDrainEnabled(GUID_CFG_LASTFM_DRAIN_ENABLED,
                                     1 // enabled by default
);

static cfg_int cfgLastfmDailyBudget(GUID_CFG_LASTFM_DAILY_BUDGET,
                                    2600 // safe default
);

static cfg_int cfgLastfmScrobblesToday(GUID_CFG_LASTFM_SCROBBLES_TODAY, 0);

static cfg_int cfgLastfmDayStamp(GUID_CFG_LASTFM_DAY_STAMP, 0);

static std::uint64_t nextQueueId()
{
    static std::uint64_t base = []() -> std::uint64_t
    {
        std::random_device rd;
        const std::uint64_t hi = static_cast<std::uint64_t>(rd());
        const std::uint64_t lo = static_cast<std::uint64_t>(rd());
        std::uint64_t v = (hi << 32) ^ lo;
        if (v == 0)
            v = 0x9e3779b97f4a7c15ull; // non-zero fallback
        return v;
    }();

    static std::atomic<std::uint64_t> seq{0};
    const std::uint64_t s = seq.fetch_add(1) + 1;
    const std::uint64_t id = base ^ (s * 0x9e3779b97f4a7c15ull);
    return id ? id : 1;
}

static bool lastfmDailyBudgetExhausted(const std::function<bool()>& isShuttingDown)
{
    if (isShuttingDown && isShuttingDown())
        return true; // treat as exhausted during shutdown: do nothing

    const std::time_t now = std::time(nullptr);
    std::tm tmNow{};
#if defined(_WIN32)
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif

    const int todayStamp = (tmNow.tm_year + 1900) * 10000 + (tmNow.tm_mon + 1) * 100 + tmNow.tm_mday;

    if (cfgLastfmDayStamp.get() != todayStamp)
    {
        if (isShuttingDown && isShuttingDown())
            return true;

        cfgLastfmDayStamp = todayStamp;
        cfgLastfmScrobblesToday = 0;
    }

    const int64_t dailyBudget = static_cast<int64_t>(cfgLastfmDailyBudget.get());
    const int64_t todayCount = static_cast<int64_t>(cfgLastfmScrobblesToday.get());
    if (dailyBudget > 0 && todayCount >= dailyBudget)
        return true;

    return false;
}
} // namespace

std::string LastfmQueue::escapeField(const std::string& in)
{
    std::string out;
    out.reserve(in.size());

    for (char c : in)
    {
        switch (c)
        {
        case '\\':
            out += "\\\\";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        default:
            out += c;
            break;
        }
    }

    return out;
}

std::string LastfmQueue::unescapeField(const std::string& in)
{
    std::string out;
    out.reserve(in.size());

    for (size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '\\' && i + 1 < in.size())
        {
            char n = in[i + 1];
            switch (n)
            {
            case '\\':
                out += '\\';
                ++i;
                continue;
            case 't':
                out += '\t';
                ++i;
                continue;
            case 'n':
                out += '\n';
                ++i;
                continue;
            case 'r':
                out += '\r';
                ++i;
                continue;
            default:
                break;
            }
        }
        out += in[i];
    }

    return out;
}

std::string LastfmQueue::serializeScrobble(const QueuedScrobble& q)
{
    std::string out;
    out += escapeField(q.artist);
    out += '\t';
    out += escapeField(q.title);
    out += '\t';
    out += escapeField(q.album);
    out += '\t';
    out += escapeField(q.albumArtist);
    out += '\t';
    out += std::to_string(q.durationSeconds);
    out += '\t';
    out += std::to_string(q.playbackSeconds);
    out += '\t';
    out += std::to_string((long long)q.startTimestamp);
    out += '\t';
    out += q.refreshOnSubmit ? "1" : "0";
    out += '\t';
    out += std::to_string(q.retryCount);
    out += '\t';
    out += std::to_string((long long)q.nextRetryTimestamp);
    out += '\t';
    out += std::to_string((unsigned long long)q.id);
    out += '\t';
    out += std::to_string(q.otherErrorCount);
    out += '\t';
    out += escapeField(q.mbid);
    return out;
}

void LastfmQueue::ensureCacheLoadedLocked() const
{
    if (cacheLoaded_)
        return;

    cache_.clear();

    pfc::string8 raw = cfgLastfmPendingScrobbles.get();
    const char* data = raw.c_str();
    if (!data || !*data)
    {
        cacheLoaded_ = true;
        return;
    }

    const char* line = data;

    // Optional header handling (FSQ2 / FSQ1). Headerless legacy is accepted for migration.
    {
        const char* nl = std::strchr(line, '\n');
        const std::string first = nl ? std::string(line, nl - line) : std::string(line);

        if (first == LastfmQueue::QUEUE_VERSION || first == "#FSQ1")
        {
            line = nl ? (nl + 1) : (line + first.size());
        }
        else if (!first.empty() && first[0] == '#')
        {
            cacheLoaded_ = true;
            return;
        }
    }

    while (*line)
    {
        const char* end = std::strchr(line, '\n');
        std::string row = end ? std::string(line, end - line) : std::string(line);
        line = end ? end + 1 : line + row.size();

        if (row.empty())
            continue;

        std::vector<std::string> parts;
        size_t pos = 0;
        while (true)
        {
            size_t tab = row.find('\t', pos);
            if (tab == std::string::npos)
            {
                parts.push_back(row.substr(pos));
                break;
            }
            parts.push_back(row.substr(pos, tab - pos));
            pos = tab + 1;
        }

        if (parts.size() < 11)
            continue;

        QueuedScrobble q;
        q.artist = unescapeField(parts[0]);
        q.title = unescapeField(parts[1]);
        q.album = unescapeField(parts[2]);
        q.albumArtist = unescapeField(parts[3]);
        q.durationSeconds = std::atof(parts[4].c_str());
        q.playbackSeconds = std::atof(parts[5].c_str());
        q.startTimestamp = static_cast<std::time_t>(std::atoll(parts[6].c_str()));
        q.refreshOnSubmit = (parts[7] == "1");
        q.retryCount = std::atoi(parts[8].c_str());
        q.nextRetryTimestamp = static_cast<std::time_t>(std::atoll(parts[9].c_str()));
        q.id = static_cast<std::uint64_t>(std::strtoull(parts[10].c_str(), nullptr, 10));
        q.otherErrorCount = (parts.size() >= 12) ? std::atoi(parts[11].c_str()) : 0;
        q.mbid = (parts.size() >= 13) ? unescapeField(parts[12]) : "";

        if (q.otherErrorCount < 0)
            q.otherErrorCount = 0;
        else if (q.otherErrorCount > 100)
            q.otherErrorCount = 100;

        if (q.id == 0)
            continue;
        if (q.artist.empty() || q.title.empty())
            continue;
        if (q.startTimestamp <= 0)
            continue;

        cache_.push_back(q);
    }

    cacheLoaded_ = true;
}

void LastfmQueue::saveCacheLocked()
{
    pfc::string8 raw;
    raw += LastfmQueue::QUEUE_VERSION;
    raw += "\n";

    for (const auto& q : cache_)
    {
        raw += serializeScrobble(q).c_str();
        raw += "\n";
    }

    cfgLastfmPendingScrobbles.set(raw);
    cacheLoaded_ = true;
}

LastfmQueue::DispatchOutcome
LastfmQueue::dispatchAndBuildRetryUpdates(const std::vector<QueuedScrobble>& snapshot, unsigned maxToAttempt,
                                          const std::function<bool()>& isShuttingDown, LastfmClient& client,
                                          const std::function<void()>& onInvalidSession, int64_t dailyBudget)
{
    const std::time_t nowCheck = std::time(nullptr);

    DispatchOutcome out;
    out.updates.reserve(maxToAttempt);

    bool invalidSessionSeen = false;
    unsigned attempted = 0;

    for (const auto& q : snapshot)
    {
        if (invalidSessionSeen || attempted >= maxToAttempt)
            break;

        if (q.nextRetryTimestamp > 0 && q.nextRetryTimestamp > nowCheck)
            continue;

        if (q.artist.empty() || q.title.empty())
        {
            LFM_INFO("Queue: pending still invalid metadata, deferring.");
            continue;
        }

        ++attempted;

        if (isShuttingDown && isShuttingDown())
            break;

        LastfmTrackInfo t;
        t.artist = q.artist;
        t.title = q.title;
        t.album = q.album;
        t.albumArtist = q.albumArtist;
        t.mbid = q.mbid;
        t.durationSeconds = q.durationSeconds;

        auto res = client.scrobble(t, q.playbackSeconds, q.startTimestamp);

        RetryUpdate u;
        u.id = q.id;
        u.newOtherErrorCount = q.otherErrorCount;

        if (res == LastfmScrobbleResult::SUCCESS)
        {
            u.remove = true;
            out.updates.push_back(u);

            if (isShuttingDown && isShuttingDown())
                break;

            cfgLastfmScrobblesToday = cfgLastfmScrobblesToday.get() + 1;

            if (dailyBudget > 0 && cfgLastfmScrobblesToday.get() >= dailyBudget)
                break;

            continue;
        }

        if (res == LastfmScrobbleResult::INVALID_SESSION)
        {
            invalidSessionSeen = true;

            if (onInvalidSession)
                onInvalidSession();
            break;
        }

        const std::time_t nowSchedule = std::time(nullptr);

        u.newRetryCount = std::min(q.retryCount + 1, 100);

        if (res == LastfmScrobbleResult::RATE_LIMITED)
        {
            u.newRetryCount = q.retryCount;
            u.newOtherErrorCount = 0;
            u.newNextRetryTimestamp = q.nextRetryTimestamp;
            out.updates.push_back(u);
            out.rateLimited = true;
            break;
        }
        else if (res == LastfmScrobbleResult::TEMPORARY_ERROR)
        {
            u.newOtherErrorCount = 0;
        }
        else if (res == LastfmScrobbleResult::OTHER_ERROR)
        {
            u.newOtherErrorCount = q.otherErrorCount + 1;

            if (u.newOtherErrorCount >= 5)
            {
                u.remove = true;
                LFM_INFO("Queue: dropping scrobble after repeated OTHER_ERRORs: "
                         << q.artist.c_str() << " - " << q.title.c_str() << " (otherErrorCount=" << u.newOtherErrorCount
                         << ")");
            }
        }
        else
        {
            u.newOtherErrorCount = q.otherErrorCount + 1;

            if (u.newOtherErrorCount >= 5)
            {
                u.remove = true;
                LFM_INFO("Queue: dropping scrobble after repeated unknown errors: "
                         << q.artist.c_str() << " - " << q.title.c_str() << " (otherErrorCount=" << u.newOtherErrorCount
                         << ")");
            }
        }

        if (!u.remove)
        {
            u.newNextRetryTimestamp =
                nowSchedule + std::min(u.newRetryCount * K_RETRY_STEP_SECONDS, K_RETRY_MAX_SECONDS);
        }

        out.updates.push_back(u);
    }

    return out;
}

void LastfmQueue::mergeRetryUpdates(std::vector<QueuedScrobble>& latest, const std::vector<RetryUpdate>& updates)
{
    for (const auto& u : updates)
    {
        for (auto it = latest.begin(); it != latest.end();)
        {
            if (it->id != u.id)
            {
                ++it;
                continue;
            }

            if (u.remove)
                it = latest.erase(it);
            else
            {
                it->retryCount = u.newRetryCount;
                it->otherErrorCount = u.newOtherErrorCount;
                it->nextRetryTimestamp = u.newNextRetryTimestamp;
                ++it;
            }

            break;
        }
    }
}

LastfmQueue::LastfmQueue(LastfmClient& client, std::function<void()> onInvalidSession)
    : client(client), onInvalidSession(std::move(onInvalidSession))
{
}

void LastfmQueue::refreshPendingScrobbleMetadata(const LastfmTrackInfo& track)
{
    std::lock_guard<std::mutex> lock(mutex);
    ensureCacheLoadedLocked();

    for (auto it = cache_.rbegin(); it != cache_.rend(); ++it)
    {
        if (!it->refreshOnSubmit)
            continue;

        LFM_DEBUG("Queue: refresh metadata");

        // Only overwrite with non-empty values
        if (!track.artist.empty())
            it->artist = track.artist;
        if (!track.title.empty())
            it->title = track.title;
        if (!track.album.empty())
            it->album = track.album;
        if (!track.albumArtist.empty())
            it->albumArtist = track.albumArtist;
        if (!track.mbid.empty())
            it->mbid = track.mbid;
        if (track.durationSeconds > 0.0)
            it->durationSeconds = track.durationSeconds;

        saveCacheLocked();
        return;
    }
}

void LastfmQueue::queueScrobbleForRetry(const LastfmTrackInfo& track, double playbackSeconds, bool refreshOnSubmit,
                                        std::time_t startTimestamp)
{
    if (track.artist.empty() || track.title.empty())
        return;

    QueuedScrobble q;
    q.artist = track.artist;
    q.title = track.title;
    q.album = track.album;
    q.albumArtist = track.albumArtist;
    q.mbid = track.mbid;
    q.durationSeconds = track.durationSeconds;
    q.playbackSeconds = playbackSeconds;
    q.startTimestamp = startTimestamp;
    q.refreshOnSubmit = refreshOnSubmit;
    q.id = nextQueueId();
    q.otherErrorCount = 0;

    std::lock_guard<std::mutex> lock(mutex);
    ensureCacheLoadedLocked();
    cache_.push_back(q);
    saveCacheLocked();

    LFM_DEBUG("Queue: queued scrobble, pending=" << (unsigned)cache_.size());
}

void LastfmQueue::enterRateLimitCooldownLocked(std::time_t now, std::time_t cooldownSeconds)
{
    if (cooldownSeconds <= 0)
        cooldownSeconds = K_RATE_LIMIT_COOLDOWN_SECONDS;

    const std::time_t until = now + cooldownSeconds;
    if (until > rateLimitedUntil_)
        rateLimitedUntil_ = until;

    if (!rateLimitLogged_)
    {
        LFM_INFO("Queue: Last.fm rate limit hit (error 29), pausing retries for "
                 << static_cast<long long>(cooldownSeconds) << "s.");
        rateLimitLogged_ = true;
    }
}

bool LastfmQueue::isRateLimitedLocked(std::time_t now)
{
    if (rateLimitedUntil_ <= 0)
        return false;

    if (now >= rateLimitedUntil_)
    {
        rateLimitedUntil_ = 0;
        rateLimitLogged_ = false;
        return false;
    }

    return true;
}

void LastfmQueue::retryQueuedScrobbles()
{
    if (core_api::is_shutting_down())
        return;

    auto isShuttingDown = [this]() -> bool { return shuttingDown_ && shuttingDown_->load(std::memory_order_acquire); };

    // IMPORTANT: do NOT touch cfg_* during shutdown, ever.
    if (isShuttingDown())
        return;

    {
        std::lock_guard<std::mutex> lock(mutex);
        if (isRateLimitedLocked(std::time(nullptr)))
            return;
    }

    if (lastfmDailyBudgetExhausted(isShuttingDown))
        return;

    const int64_t dailyBudget = static_cast<int64_t>(cfgLastfmDailyBudget.get());
    const int64_t todayCount = static_cast<int64_t>(cfgLastfmScrobblesToday.get());

    int64_t remaining = (dailyBudget > 0) ? (dailyBudget - todayCount) : INT64_MAX;
    remaining = std::max<int64_t>(0, remaining);

    if (dailyBudget > 0 && remaining <= 0)
        return;

    const unsigned maxToAttempt = (dailyBudget > 0)
                                      ? (unsigned)std::min<int64_t>((int64_t)K_MAX_DISPATCH_BATCH, remaining)
                                      : (unsigned)K_MAX_DISPATCH_BATCH;

    std::vector<QueuedScrobble> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex);
        ensureCacheLoadedLocked();
        snapshot = cache_;
    }

    if (snapshot.empty())
        return;

    const auto dispatch =
        dispatchAndBuildRetryUpdates(snapshot, maxToAttempt, isShuttingDown, client, onInvalidSession, dailyBudget);

    if (isShuttingDown())
        return;

    if (dispatch.rateLimited)
    {
        std::lock_guard<std::mutex> lock(mutex);
        enterRateLimitCooldownLocked(std::time(nullptr), K_RATE_LIMIT_COOLDOWN_SECONDS);
    }

    if (dispatch.updates.empty())
        return;

    if (isShuttingDown())
        return;

    std::lock_guard<std::mutex> lock(mutex);
    ensureCacheLoadedLocked();
    mergeRetryUpdates(cache_, dispatch.updates);

    if (isShuttingDown())
        return;

    saveCacheLocked();
    LFM_DEBUG("Queue: merge-save done, pending=" << (unsigned)cache_.size());
}

std::size_t LastfmQueue::getPendingScrobbleCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    ensureCacheLoadedLocked();
    return cache_.size();
}

bool LastfmQueue::hasDueScrobble(std::time_t now)
{
    std::lock_guard<std::mutex> lock(mutex);
    ensureCacheLoadedLocked();
    if (isRateLimitedLocked(now))
        return false;

    for (const auto& q : cache_)
        if (q.nextRetryTimestamp == 0 || q.nextRetryTimestamp <= now)
            return true;
    return false;
}

void LastfmQueue::clearAll()
{
    std::lock_guard<std::mutex> lock(mutex);
    cache_.clear();
    cacheLoaded_ = true;
    saveCacheLocked();
    rateLimitedUntil_ = 0;
    rateLimitLogged_ = false;
    LFM_INFO("Queue: cleared all pending scrobbles.");
}

bool LastfmQueue::drainEnabled()
{
    return cfgLastfmDrainEnabled.get() != 0;
}

std::chrono::seconds LastfmQueue::drainCooldown()
{
    int64_t cooldown = cfgLastfmDrainCooldownSeconds.get();

    if (cooldown < 0)
        cooldown = 0;
    else if (cooldown > K_RETRY_MAX_SECONDS)
        cooldown = K_RETRY_MAX_SECONDS;

    return std::chrono::seconds(cooldown);
}
