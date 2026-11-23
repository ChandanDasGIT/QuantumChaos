#pragma once

#include "common/notelibrary.h"
#include "common/platform/httpclient.h"
#include "common/platform/scheduler.h"

#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>

// Keeps this device's notes the same as the user's sync server, and so the same as their other
// devices. The server keeps each note as bytes sealed with the app key (it never sees a note) and
// gives every change the next revision number.
//   pull: GET /changes?since=<rev>, download what changed, merge it with this device's version
//   push: PUT each note edited here with the revision it was based on (409: another device changed
//         it first: download, merge, try again); DELETE the notes deleted for good here
// While the app is in front it also waits for changes (a long poll), so edits made on another
// device show up at once. What was synced is kept in a small state file, so edits made offline go
// up later and nothing is downloaded twice.
namespace devicesync {

class ServerSync {
public:
    enum class Status { Off, Syncing, Synced, Offline, Error };

    ServerSync(NoteLibrary& library, platform::HttpClient& http, platform::Scheduler& scheduler,
               std::filesystem::path stateFile);
    ServerSync(const ServerSync&) = delete;
    ServerSync& operator=(const ServerSync&) = delete;

    // address: "https://host"; token: the server's secret. Either empty: sync stays off.
    void start(const std::string& address, const std::string& token);
    // The app came to the front (sync, then wait for changes) or went to the back (stop waiting).
    void setActive(bool active);
    void syncNow();

    Status status() const { return m_status; }
    // Why the last sync failed, or what it couldn't do; empty when all went well.
    const std::string& problem() const { return m_problem; }
    std::int64_t lastSyncMs() const { return m_lastSyncMs; }
    std::function<void()> onStatusChanged;

    static constexpr std::int64_t writeDelayMs = 1'500;     // after an edit, before it goes up
    static constexpr int pollTimeoutMs = 40'000;            // the server answers a long poll within 25 s
    static constexpr std::int64_t retryFirstMs = 2'000;
    static constexpr std::int64_t retryMaxMs = 60'000;
    static constexpr int conflictTries = 3;

private:
    struct Change {
        std::string id;
        std::int64_t rev = 0;
        bool deleted = false;
    };

    bool running() const { return !m_address.empty(); }
    platform::HttpRequest request(const std::string& method, const std::string& path) const;
    void send(platform::HttpRequest request, std::function<void(const platform::HttpResponse&)> then);

    void runCycle();
    void pull(std::int64_t since);
    void nextChange();
    void fetch(const std::string& id, std::int64_t rev, std::function<void()> then);
    void remoteArrived(const std::string& id, std::int64_t rev, const std::string& sealed);
    void remoteDeleted(const std::string& id, std::int64_t rev);
    void pushNext();
    void pushNote(const std::string& id);
    void pushDelete(const std::string& id);
    void conflict(const std::string& id, const platform::HttpResponse& response);
    void finish(const std::string& problem = {});
    void failed(const platform::HttpResponse& response);
    void poll();
    void retryLater();
    void edited(const std::string& id);

    void loadState();
    void saveState();
    void setStatus(Status status, const std::string& problem = {});

    NoteLibrary& m_library;
    platform::HttpClient& m_http;
    platform::Scheduler& m_scheduler;
    std::filesystem::path m_stateFile;
    std::string m_address;
    std::string m_token;

    // Kept between runs: the server revision synced up to, each note's revision as last seen on
    // the server, and the notes edited here that haven't gone up yet.
    std::int64_t m_lastRev = 0;
    std::map<std::string, std::int64_t> m_known;
    std::set<std::string> m_dirty;

    bool m_active = false;
    bool m_busy = false;
    bool m_again = false;
    bool m_polling = false;
    bool m_writePending = false;
    int m_generation = 0;
    int m_pollGeneration = 0;
    std::int64_t m_retryMs = retryFirstMs;
    std::deque<Change> m_changes;
    std::int64_t m_pullRev = 0;
    std::set<std::string> m_pushed;          // tried in this round already
    std::map<std::string, int> m_conflicts;
    int m_unopenable = 0;

    Status m_status = Status::Off;
    std::string m_problem;
    std::int64_t m_lastSyncMs = 0;
    platform::Lifetime m_lifetime;
};

} // namespace devicesync
