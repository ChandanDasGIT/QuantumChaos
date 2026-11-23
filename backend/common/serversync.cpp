#include "common/serversync.h"

#include "common/files.h"
#include "common/json.h"
#include "common/notemerge.h"

#include <algorithm>

namespace devicesync {

namespace {

bool unreachable(const platform::HttpResponse& r)
{
    return !r.error.empty() || r.status == 0 || r.status == 500 || r.status == 502 || r.status == 503 ||
           r.status == 504;
}

const std::string unreachableText = "Can't reach the sync server right now; it tries again by itself";

} // namespace

ServerSync::ServerSync(NoteLibrary& library, platform::HttpClient& http, platform::Scheduler& scheduler,
                       std::filesystem::path stateFile)
    : m_library(library), m_http(http), m_scheduler(scheduler), m_stateFile(std::move(stateFile))
{
    loadState();
    m_library.onEdited = [this](const std::string& id) { edited(id); };
    m_library.onErased = [this](const std::string& id) { edited(id); };
}

void ServerSync::start(const std::string& address, const std::string& token)
{
    ++m_generation;
    ++m_pollGeneration;
    m_busy = m_again = m_polling = m_writePending = false;
    m_address = address;
    while (!m_address.empty() && m_address.back() == '/') m_address.pop_back();
    m_token = token;
    if (m_address.empty() || m_token.empty()) {
        m_address.clear();
        setStatus(Status::Off);
        return;
    }
    // Notes that never reached this server go up on the first sync.
    for (const auto& n : m_library.list().all())
        if (!m_known.contains(n.id)) m_dirty.insert(n.id);
    saveState();
    if (m_active) runCycle();
}

void ServerSync::setActive(bool active)
{
    if (active == m_active) return;
    m_active = active;
    if (!active) {
        ++m_pollGeneration;
        m_polling = false;
        return;
    }
    runCycle();
}

void ServerSync::syncNow()
{
    runCycle();
}

platform::HttpRequest ServerSync::request(const std::string& method, const std::string& path) const
{
    platform::HttpRequest r;
    r.method = method;
    r.url = m_address + path;
    r.headers.push_back({"Authorization", "Bearer " + m_token});
    return r;
}

void ServerSync::send(platform::HttpRequest request, std::function<void(const platform::HttpResponse&)> then)
{
    const int generation = m_generation;
    auto alive = m_lifetime.watch();
    m_http.get(request, [this, generation, alive, then = std::move(then)](platform::HttpResponse response) {
        if (alive.expired() || generation != m_generation) return;
        then(response);
    });
}

void ServerSync::runCycle()
{
    if (!running()) return;
    if (m_busy) {
        m_again = true;
        return;
    }
    m_busy = true;
    m_pushed.clear();
    m_conflicts.clear();
    m_unopenable = 0;
    setStatus(Status::Syncing);
    pull(m_lastRev);
}

void ServerSync::pull(std::int64_t since)
{
    send(request("GET", "/changes?since=" + std::to_string(since)), [this, since](const platform::HttpResponse& r) {
        if (!r.ok()) return failed(r);
        const auto parsed = json::parse(r.body);
        if (!parsed.ok() || !parsed.value["rev"].isNumber() || !parsed.value["changes"].isArray())
            return finish("The sync server sent an answer the app can't read");
        const std::int64_t rev = parsed.value["rev"].asInt();
        if (since > rev) return pull(0);            // the server was restored from a backup
        m_changes.clear();
        for (const auto& c : parsed.value["changes"].asArray())
            m_changes.push_back({c["id"].asString(), c["rev"].asInt(), c["deleted"].asBool()});
        m_pullRev = rev;
        nextChange();
    });
}

void ServerSync::nextChange()
{
    while (!m_changes.empty()) {
        const Change c = m_changes.front();
        m_changes.pop_front();
        const auto known = m_known.find(c.id);
        if (known != m_known.end() && known->second == c.rev) continue;    // this device's own upload
        if (c.deleted) {
            remoteDeleted(c.id, c.rev);
            continue;
        }
        fetch(c.id, c.rev, [this] { nextChange(); });
        return;
    }
    m_lastRev = std::max(m_lastRev, m_pullRev);
    saveState();
    pushNext();
}

void ServerSync::fetch(const std::string& id, std::int64_t rev, std::function<void()> then)
{
    send(request("GET", "/notes/" + id), [this, id, rev, then = std::move(then)](const platform::HttpResponse& r) {
        if (r.status == 404 || r.status == 410) {
            remoteDeleted(id, rev);
            return then();
        }
        if (!r.ok()) return failed(r);
        remoteArrived(id, rev, r.body);
        then();
    });
}

void ServerSync::remoteArrived(const std::string& id, std::int64_t rev, const std::string& sealed)
{
    m_known[id] = rev;
    const auto remote = m_library.store().open(id, sealed);
    if (!remote) {
        ++m_unopenable;
        saveState();
        return;
    }
    const Note* local = m_library.list().find(id);
    const Note merged = local ? notemerge::merge(*local, *remote) : *remote;
    if (!local || merged != *local) m_library.applyRemote(merged);
    if (merged != *remote)
        m_dirty.insert(id);         // this device's changes still have to go up
    else
        m_dirty.erase(id);
    saveState();
}

void ServerSync::remoteDeleted(const std::string& id, std::int64_t rev)
{
    m_known[id] = rev;
    // Edited here but not sent yet: the edit wins and brings the note back on the server.
    if (!(m_dirty.contains(id) && m_library.list().find(id))) {
        m_dirty.erase(id);
        if (m_library.list().find(id)) m_library.removeRemote(id);
    }
    saveState();
}

void ServerSync::pushNext()
{
    const auto next = std::find_if(m_dirty.begin(), m_dirty.end(), [this](const std::string& id) {
        return !m_pushed.contains(id);
    });
    if (next == m_dirty.end()) {
        if (m_unopenable > 0)
            return finish(std::to_string(m_unopenable) +
                          (m_unopenable == 1 ? " note" : " notes") + " on the server can't be opened with this app's key");
        return finish();
    }
    const std::string id = *next;
    m_pushed.insert(id);
    if (m_library.list().find(id))
        pushNote(id);
    else
        pushDelete(id);
}

void ServerSync::pushNote(const std::string& id)
{
    const Note* n = m_library.list().find(id);
    auto r = request("PUT", "/notes/" + id);
    const auto known = m_known.find(id);
    r.headers.push_back({"X-Base-Rev", std::to_string(known == m_known.end() ? 0 : known->second)});
    r.headers.push_back({"Content-Type", "application/octet-stream"});
    r.body = m_library.store().seal(*n);
    // An edit made while this is on its way marks the note again.
    m_dirty.erase(id);
    saveState();
    send(std::move(r), [this, id](const platform::HttpResponse& response) {
        if (response.ok()) {
            m_known[id] = json::parse(response.body).value["rev"].asInt();
            saveState();
            return pushNext();
        }
        m_dirty.insert(id);
        saveState();
        if (response.status == 409) return conflict(id, response);
        if (response.status == 413) return finish("A note is too big for the sync server (it takes up to 1 MB)");
        failed(response);
    });
}

void ServerSync::pushDelete(const std::string& id)
{
    const auto known = m_known.find(id);
    if (known == m_known.end()) {                   // it never reached the server
        m_dirty.erase(id);
        saveState();
        return pushNext();
    }
    auto r = request("DELETE", "/notes/" + id);
    r.headers.push_back({"X-Base-Rev", std::to_string(known->second)});
    m_dirty.erase(id);
    saveState();
    send(std::move(r), [this, id](const platform::HttpResponse& response) {
        if (response.ok()) {
            m_known[id] = json::parse(response.body).value["rev"].asInt();
            saveState();
            return pushNext();
        }
        if (response.status == 404) {
            m_known.erase(id);
            saveState();
            return pushNext();
        }
        m_dirty.insert(id);
        saveState();
        if (response.status == 409) return conflict(id, response);
        failed(response);
    });
}

void ServerSync::conflict(const std::string& id, const platform::HttpResponse& response)
{
    const auto parsed = json::parse(response.body);
    const std::int64_t rev = parsed.value["rev"].asInt();
    if (++m_conflicts[id] > conflictTries) return finish("A note kept changing on another device; it tries again later");
    m_pushed.erase(id);
    if (parsed.value["deleted"].asBool()) {
        m_known[id] = rev;
        // Deleted there and here: nothing left to do. Edited here: sending it again brings it back.
        if (!m_library.list().find(id)) m_dirty.erase(id);
        saveState();
        return pushNext();
    }
    fetch(id, rev, [this] { pushNext(); });
}

void ServerSync::finish(const std::string& problem)
{
    m_busy = false;
    m_retryMs = retryFirstMs;
    m_lastSyncMs = m_scheduler.nowMs();
    setStatus(problem.empty() ? Status::Synced : Status::Error, problem);
    if (m_again) {
        m_again = false;
        return runCycle();
    }
    poll();
}

void ServerSync::failed(const platform::HttpResponse& r)
{
    m_busy = false;
    m_again = false;
    if (unreachable(r)) {
        setStatus(Status::Offline, unreachableText);
        return retryLater();
    }
    // These need a fix on the server or in the build: no point trying again by itself.
    if (r.status == 401)
        setStatus(Status::Error, "The sync server refused this app's token");
    else if (r.status == 403)
        setStatus(Status::Error, "The sync server only answers over https");
    else if (r.status == 507)
        setStatus(Status::Error, "The sync server is full (it keeps up to 100 000 notes)");
    else
        setStatus(Status::Error, "The sync server answered " + std::to_string(r.status));
}

void ServerSync::poll()
{
    if (!running() || !m_active || m_polling || m_busy) return;
    m_polling = true;
    const int poll = ++m_pollGeneration;
    const std::int64_t since = m_lastRev;
    auto r = request("GET", "/changes?since=" + std::to_string(since) + "&wait=1");
    r.timeoutMs = pollTimeoutMs;
    send(std::move(r), [this, poll, since](const platform::HttpResponse& response) {
        if (poll != m_pollGeneration) return;
        m_polling = false;
        if (!response.ok()) return failed(response);
        const auto parsed = json::parse(response.body);
        const bool changed = !parsed.value["changes"].asArray().empty() || since > parsed.value["rev"].asInt();
        if (changed)
            runCycle();
        else
            this->poll();
    });
}

void ServerSync::retryLater()
{
    const int generation = m_generation;
    auto alive = m_lifetime.watch();
    const std::int64_t delay = m_retryMs;
    m_retryMs = std::min(m_retryMs * 2, retryMaxMs);
    m_scheduler.after(delay, [this, generation, alive] {
        if (alive.expired() || generation != m_generation) return;
        runCycle();
    });
}

void ServerSync::edited(const std::string& id)
{
    m_dirty.insert(id);
    saveState();
    if (!running() || m_writePending) return;
    m_writePending = true;
    const int generation = m_generation;
    auto alive = m_lifetime.watch();
    m_scheduler.after(writeDelayMs, [this, generation, alive] {
        if (alive.expired() || generation != m_generation) return;
        m_writePending = false;
        runCycle();
    });
}

void ServerSync::loadState()
{
    const auto bytes = files::read(m_stateFile);
    if (!bytes) return;
    const auto parsed = json::parse(*bytes);
    if (!parsed.ok()) return;
    m_lastRev = parsed.value["lastRev"].asInt();
    for (const auto& [id, rev] : parsed.value["known"].asObject()) m_known[id] = rev.asInt();
    for (const auto& id : parsed.value["dirty"].asArray()) m_dirty.insert(id.asString());
}

void ServerSync::saveState()
{
    json::Value::Object known;
    known.reserve(m_known.size());
    for (const auto& [id, rev] : m_known) known.emplace_back(id, json::Value(rev));
    json::Value dirty = json::Value::array();
    for (const auto& id : m_dirty) dirty.push(id);
    json::Value v = json::Value::object();
    v.set("lastRev", m_lastRev);
    v.set("known", json::Value(std::move(known)));
    v.set("dirty", std::move(dirty));
    files::write(m_stateFile, json::write(v));
}

void ServerSync::setStatus(Status status, const std::string& problem)
{
    m_status = status;
    m_problem = problem;
    if (onStatusChanged) onStatusChanged();
}

} // namespace devicesync
