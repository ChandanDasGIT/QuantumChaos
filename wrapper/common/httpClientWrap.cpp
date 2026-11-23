#include "httpClientWrap.h"

#include <QCoreApplication>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

HttpClientWrap::HttpClientWrap(QObject* parent) : QObject(parent)
{
}

QByteArray HttpClientWrap::userAgent()
{
    return QByteArrayLiteral("QuantumChaos/") + QCoreApplication::applicationVersion().toUtf8();
}

void HttpClientWrap::get(const platform::HttpRequest& request, std::function<void(platform::HttpResponse)> done)
{
    QNetworkRequest qrequest(QUrl(QString::fromStdString(request.url)));
    qrequest.setHeader(QNetworkRequest::UserAgentHeader, userAgent());
    qrequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    qrequest.setTransferTimeout(request.timeoutMs > 0 ? request.timeoutMs : 30000);
    // Qt's HTTP/2 code logs "device not open" when a server closes a connection; HTTP/1.1 does not.
    qrequest.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    for (const auto& [name, value] : request.headers)
        qrequest.setRawHeader(QByteArray::fromStdString(name), QByteArray::fromStdString(value));

    QNetworkReply* reply = request.method == "GET"
        ? m_network.get(qrequest)
        : m_network.sendCustomRequest(qrequest, QByteArray::fromStdString(request.method),
                                      QByteArray::fromStdString(request.body));
    connect(reply, &QNetworkReply::finished, this, [reply, done = std::move(done)] {
        platform::HttpResponse response;
        response.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        // A reply that failed before any answer (a timeout, a refused connection) is already closed.
        if (reply->isOpen()) {
            const QByteArray body = reply->readAll();
            response.body.assign(body.constData(), static_cast<std::size_t>(body.size()));
        }
        // An HTTP error status is not a network error: the body and status still arrive.
        if (reply->error() != QNetworkReply::NoError && response.status == 0)
            response.error = reply->errorString().toStdString();
        reply->deleteLater();
        done(std::move(response));
    });
}
