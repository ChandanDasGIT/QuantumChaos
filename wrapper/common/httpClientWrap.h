#pragma once

#include "common/platform/httpclient.h"

#include <QNetworkAccessManager>
#include <QObject>

// The backend's HttpClient on Qt's network stack. Answers arrive on the thread that owns this
// object, from the event loop. Every request carries the app's User-Agent.
class HttpClientWrap : public QObject, public platform::HttpClient {
    Q_OBJECT

public:
    explicit HttpClientWrap(QObject* parent = nullptr);

    void get(const platform::HttpRequest& request, std::function<void(platform::HttpResponse)> done) override;

    static QByteArray userAgent();

private:
    QNetworkAccessManager m_network;
};
