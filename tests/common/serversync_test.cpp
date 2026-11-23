#include "testkit.h"

#include "common/json.h"
#include "common/serversync.h"

#include <algorithm>
#include <deque>
#include <map>
#include <memory>

using devicesync::ServerSync;

namespace {

const crypto::Key key = *crypto::keyFromHex(std::string(64, '7'));
const std::string address = "https://sync.test";

std::string header(const platform::HttpRequest& r, const std::string& name)
{
    for (const auto& [k, v] : r.headers)
        if (k == name) return v;
    return {};
}

// The sync server's rules, kept in memory.
struct FakeServer {
    struct Entry {
        std::int64_t rev = 0;
        std::string blob;
        bool deleted = false;
    };
    std::map<std::string, Entry> notes;
    std::int64_t rev = 0;
    std::string token = "secret";
    bool down = false;
    int noteDownloads = 0;
    int conflicts = 0;

    platform::HttpResponse answer(const platform::HttpRequest& r)
    {
        if (down) return {0, {}, "Connection refused"};
        if (header(r, "Authorization") != "Bearer " + token) return {401, R"({"error":"token"})", {}};
        const std::string path = r.url.substr(address.size());
        if (r.method == "GET" && path.starts_with("/changes?since=")) {
            const std::int64_t since = std::stoll(path.substr(15));
            std::vector<std::pair<std::int64_t, std::string>> changed;
            for (const auto& [id, e] : notes)
                if (e.rev > since) changed.push_back({e.rev, id});
            std::sort(changed.begin(), changed.end());
            json::Value list = json::Value::array();
            for (const auto& [rev, id] : changed) {
                json::Value c = json::Value::object();
                c.set("id", id);
                c.set("rev", rev);
                c.set("size", static_cast<std::int64_t>(notes[id].blob.size()));
                c.set("deleted", notes[id].deleted);
                list.push(std::move(c));
            }
            json::Value v = json::Value::object();
            v.set("rev", this->rev);
            v.set("changes", std::move(list));
            return {200, json::write(v), {}};
        }
        if (!path.starts_with("/notes/")) return {400, {}, {}};
        const std::string id = path.substr(7);
        const auto it = notes.find(id);
        if (r.method == "GET") {
            if (it == notes.end()) return {404, {}, {}};
            if (it->second.deleted) return {410, R"({"error":"deleted","rev":)" + std::to_string(it->second.rev) + "}", {}};
            ++noteDownloads;
            return {200, it->second.blob, {}};
        }
        const std::int64_t current = it == notes.end() ? 0 : it->second.rev;
        if (std::stoll(header(r, "X-Base-Rev")) != current) {
            ++conflicts;
            json::Value v = json::Value::object();
            v.set("error", "conflict");
            v.set("rev", current);
            v.set("deleted", it != notes.end() && it->second.deleted);
            return {409, json::write(v), {}};
        }
        if (r.method == "PUT") {
            if (r.body.size() > 1024 * 1024) return {413, R"({"error":"too_large"})", {}};
            notes[id] = {++rev, r.body, false};
            return {200, R"({"id":")" + id + R"(","rev":)" + std::to_string(rev) + "}", {}};
        }
        if (r.method == "DELETE") {
            if (it == notes.end()) return {404, {}, {}};
            notes[id] = {++rev, {}, true};
            return {200, R"({"id":")" + id + R"(","rev":)" + std::to_string(rev) + R"(,"deleted":true})", {}};
        }
        return {400, {}, {}};
    }
};

// Answers requests only when the test pumps it. A long poll (wait=1) waits until the server changes.
struct FakeNet : platform::HttpClient {
    using Pending = std::pair<platform::HttpRequest, std::function<void(platform::HttpResponse)>>;
    explicit FakeNet(FakeServer& s) : server(s) {}
    FakeServer& server;
    std::deque<Pending> queue;
    std::vector<std::pair<std::int64_t, Pending>> parked;

    void get(const platform::HttpRequest& request, std::function<void(platform::HttpResponse)> done) override
    {
        queue.push_back({request, std::move(done)});
    }

    bool pumpOne()
    {
        for (auto it = parked.begin(); it != parked.end(); ++it) {
            if (server.down || server.rev > it->first) {
                queue.push_back(std::move(it->second));
                parked.erase(it);
                break;
            }
        }
        if (queue.empty()) return false;
        Pending p = std::move(queue.front());
        queue.pop_front();
        if (p.first.url.find("&wait=1") != std::string::npos && !server.down) {
            const std::int64_t since = std::stoll(p.first.url.substr(p.first.url.find("since=") + 6));
            if (server.rev <= since) {
                parked.push_back({since, std::move(p)});
                return true;
            }
        }
        p.second(server.answer(p.first));
        return true;
    }

    void pump()
    {
        for (int guard = 0; guard < 100000 && pumpOne(); ++guard) {}
    }
};

struct FakeScheduler : platform::Scheduler {
    std::int64_t now = 1000;
    std::vector<std::pair<std::int64_t, std::function<void()>>> jobs;

    std::int64_t nowMs() const override { return now; }
    void after(std::int64_t delayMs, std::function<void()> fn) override { jobs.push_back({now + delayMs, std::move(fn)}); }
    void post(std::function<void()> fn) override { after(0, std::move(fn)); }

    void advance(std::int64_t ms)
    {
        now += ms;
        for (bool ran = true; ran;) {
            ran = false;
            for (auto it = jobs.begin(); it != jobs.end(); ++it) {
                if (it->first <= now) {
                    auto fn = std::move(it->second);
                    jobs.erase(it);
                    fn();
                    ran = true;
                    break;
                }
            }
        }
    }
};

// One device: its own folder, clock and network, sharing the one server.
struct Device {
    Device(FakeServer& server, const std::filesystem::path& folder, const std::string& token = "secret")
        : net(server), library(folder, key, [this] { return scheduler.now; }),
          sync(library, net, scheduler, folder / "sync.json")
    {
        sync.start(address, token);
        sync.setActive(true);
        settle();
    }

    // Lets every request, retry and delayed upload run until the device is quiet.
    void settle()
    {
        for (int i = 0; i < 30; ++i) {
            net.pump();
            scheduler.advance(ServerSync::writeDelayMs);
        }
        net.pump();
    }

    notes::NoteList& list() { return library.list(); }

    FakeScheduler scheduler;
    FakeNet net;
    NoteLibrary library;
    ServerSync sync;
};

} // namespace

TEST(serversync, a_note_made_on_one_device_shows_on_the_other)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setTitle(id, "Groceries");
    a.list().setBody(id, "Milk\nBread ☕");
    a.settle();
    CHECK(a.sync.status() == ServerSync::Status::Synced);
    CHECK_EQ(server.notes.size(), std::size_t(1));
    CHECK(server.notes[id].blob.find("Groceries") == std::string::npos);    // the server only sees sealed bytes

    Device b(server, dirB.path());
    const Note* n = b.list().find(id);
    CHECK(n != nullptr);
    CHECK(*n == *a.list().find(id));
}

TEST(serversync, an_edit_shows_up_at_once_through_the_long_poll)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    Device b(server, dirB.path());
    CHECK_EQ(b.net.parked.size(), std::size_t(1));                      // b is waiting for changes
    const std::string id = a.list().create(false);
    a.list().setBody(id, "hello");
    a.settle();
    b.net.pump();                                                        // no timers: only the waiting poll answers
    CHECK(b.list().find(id) != nullptr);
    CHECK_EQ(b.list().find(id)->body, std::string("hello"));
}

TEST(serversync, edits_to_different_parts_on_two_devices_both_stay)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "start");
    a.settle();
    Device b(server, dirB.path());

    // b sends its edit, but a's edit reaches the server first: b gets a conflict (409) and merges.
    b.scheduler.now = 5'000'000;                                         // clocks only move forward
    b.list().setBody(id, "body from b");
    b.scheduler.advance(ServerSync::writeDelayMs);
    b.net.pumpOne();                                                     // b's pull: nothing new yet
    a.scheduler.now = 6'000'000;
    a.list().setTitle(id, "title from a");
    a.settle();
    b.settle();
    a.settle();
    for (Device* d : {&a, &b}) {
        const Note* n = d->list().find(id);
        CHECK_EQ(n->title, std::string("title from a"));
        CHECK_EQ(n->body, std::string("body from b"));
    }
    CHECK(*a.list().find(id) == *b.list().find(id));
    CHECK(server.conflicts > 0);                                        // the 409 path really ran
}

TEST(serversync, the_newer_change_to_one_part_wins_on_both)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "x");
    a.settle();
    Device b(server, dirB.path());
    a.scheduler.now = 7'000'000;
    a.list().setTitle(id, "older");
    b.scheduler.now = 9'000'000;
    b.list().setTitle(id, "newer");
    a.settle();
    b.settle();
    a.settle();
    CHECK_EQ(a.list().find(id)->title, std::string("newer"));
    CHECK_EQ(b.list().find(id)->title, std::string("newer"));
}

TEST(serversync, a_note_deleted_for_good_goes_from_every_device)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "bin me");
    a.settle();
    Device b(server, dirB.path());
    CHECK(b.list().find(id) != nullptr);
    a.list().setTrashed(id, true);
    a.list().emptyTrash();
    a.settle();
    CHECK(server.notes[id].deleted);
    b.settle();
    CHECK(b.list().find(id) == nullptr);
    CHECK(!std::filesystem::exists(dirB.path() / "notes" / (id + ".note")));
}

TEST(serversync, an_unsent_edit_brings_back_a_note_deleted_elsewhere)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "keep?");
    a.settle();
    Device b(server, dirB.path());
    server.down = true;
    b.list().setBody(id, "yes, keep it");
    b.settle();
    server.down = false;
    a.list().erase(id);
    a.settle();
    b.scheduler.advance(ServerSync::retryMaxMs);
    b.settle();
    a.settle();
    CHECK(!server.notes[id].deleted);
    CHECK(a.list().find(id) != nullptr);
    CHECK_EQ(a.list().find(id)->body, std::string("yes, keep it"));
}

TEST(serversync, edits_made_offline_go_up_when_the_server_is_back)
{
    FakeServer server;
    testkit::TempFolder dirA;
    Device a(server, dirA.path());
    server.down = true;
    const std::string id = a.list().create(false);
    a.list().setBody(id, "written offline");
    a.settle();
    CHECK(a.sync.status() == ServerSync::Status::Offline);
    CHECK(server.notes.empty());
    server.down = false;
    a.scheduler.advance(ServerSync::retryMaxMs);
    a.settle();
    CHECK(a.sync.status() == ServerSync::Status::Synced);
    CHECK_EQ(server.notes.size(), std::size_t(1));
}

TEST(serversync, what_was_synced_is_remembered_after_a_restart)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB;
    std::string id;
    {
        Device a(server, dirA.path());
        id = a.list().create(false);
        a.list().setBody(id, "once");
        a.settle();
    }
    {
        Device b(server, dirB.path());
    }
    const int downloads = server.noteDownloads;
    const std::int64_t revs = server.rev;
    Device a(server, dirA.path());
    Device b(server, dirB.path());
    CHECK_EQ(server.noteDownloads, downloads);                          // nothing downloaded twice
    CHECK_EQ(server.rev, revs);                                         // nothing sent twice
    CHECK(b.list().find(id) != nullptr);
}

TEST(serversync, edits_waiting_to_go_up_survive_a_restart)
{
    FakeServer server;
    testkit::TempFolder dirA;
    std::string id;
    server.down = true;
    {
        Device a(server, dirA.path());
        id = a.list().create(false);
        a.list().setBody(id, "made while offline, then the app closed");
    }
    server.down = false;
    Device a(server, dirA.path());
    CHECK_EQ(server.notes.size(), std::size_t(1));
    CHECK(server.notes.contains(id));
}

TEST(serversync, a_wrong_token_is_an_error_without_retrying)
{
    FakeServer server;
    testkit::TempFolder dirA;
    Device a(server, dirA.path(), "wrong");
    CHECK(a.sync.status() == ServerSync::Status::Error);
    CHECK(a.sync.problem().find("token") != std::string::npos);
    CHECK(a.scheduler.jobs.empty());
}

TEST(serversync, a_note_sealed_with_another_key_is_reported_and_the_rest_syncs)
{
    FakeServer server;
    testkit::TempFolder dirA, dirB, dirOther;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "fine");
    a.settle();
    {
        NoteStore other(dirOther.path(), *crypto::keyFromHex(std::string(64, '8')));
        Note stranger;
        stranger.id = "cccccccccccccccccccccccccccccccc";
        server.notes[stranger.id] = {++server.rev, other.seal(stranger), false};
    }
    Device b(server, dirB.path());
    CHECK(b.list().find(id) != nullptr);
    CHECK(b.list().find("cccccccccccccccccccccccccccccccc") == nullptr);
    CHECK(b.sync.status() == ServerSync::Status::Error);
    CHECK(b.sync.problem().find("key") != std::string::npos);
}

TEST(serversync, a_server_restored_from_a_backup_is_read_again)
{
    FakeServer server;
    testkit::TempFolder dirA;
    Device a(server, dirA.path());
    const std::string id = a.list().create(false);
    a.list().setBody(id, "x");
    a.settle();
    server.notes.clear();
    server.rev = 0;
    a.sync.syncNow();
    a.settle();
    CHECK(a.sync.status() == ServerSync::Status::Synced);
    CHECK(a.list().find(id) != nullptr);
}

TEST(serversync, without_a_server_sync_stays_off)
{
    FakeServer server;
    testkit::TempFolder dirA;
    FakeScheduler scheduler;
    FakeNet net(server);
    NoteLibrary library(dirA.path(), key, [&] { return scheduler.now; });
    ServerSync sync(library, net, scheduler, dirA.path() / "sync.json");
    sync.start("", "");
    sync.setActive(true);
    library.list().create(false);
    net.pump();
    scheduler.advance(ServerSync::writeDelayMs);
    CHECK(sync.status() == ServerSync::Status::Off);
    CHECK(net.queue.empty());
}
