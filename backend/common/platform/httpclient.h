#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// What the backend needs from the host to talk to the web. The app gives it a Qt implementation,
// the tests a fake one. Everything happens on one thread: `done` is called later from the host's
// event loop, never from inside get().
namespace platform {

struct HttpRequest {
    std::string url;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string method = "GET";     // "GET", "PUT", "POST", "DELETE"
    std::string body = {};          // sent with PUT and POST
    int timeoutMs = 0;              // how long without data before giving up (0: the host's usual)
};

struct HttpResponse {
    int status = 0;             // 0 when no answer arrived
    std::string body;
    std::string error;          // network trouble: "Connection refused", "Operation timed out", ...
    bool ok() const { return error.empty() && status >= 200 && status < 300; }
    // A short reason for the user when !ok(): the network error, or "the site answered 404".
    std::string problem() const;
};

class HttpClient {
public:
    virtual ~HttpClient() = default;
    // Sends the request with its method (GET unless it says otherwise).
    virtual void get(const HttpRequest& request, std::function<void(HttpResponse)> done) = 0;
};

// Answers can arrive after whoever asked is gone. An object keeps a Lifetime and hands out
// lifetime.watch(); a callback checks `if (watch.expired()) return;` before touching the object.
class Lifetime {
public:
    std::weak_ptr<char> watch() const { return m_alive; }

private:
    std::shared_ptr<char> m_alive = std::make_shared<char>();
};

inline std::string HttpResponse::problem() const
{
    if (!error.empty()) return error;
    if (status == 0) return "no answer from the site";
    return "the site answered " + std::to_string(status);
}

} // namespace platform
