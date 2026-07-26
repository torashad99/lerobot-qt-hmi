#include "TelemetryWorker.h"

#include <chrono>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Protocol.h"

TelemetryWorker::TelemetryWorker(zmq::context_t *ctx, QObject *parent)
    : QObject(parent), m_ctx(ctx) {}

void TelemetryWorker::configure(const QString &endpoint) {
    m_endpoint = endpoint;
}

void TelemetryWorker::run() {
    zmq::socket_t sub(*m_ctx, zmq::socket_type::sub);
    sub.set(zmq::sockopt::subscribe, "");
    sub.set(zmq::sockopt::rcvhwm, 16);
    try {
        sub.connect(m_endpoint.toStdString());
    } catch (const zmq::error_t &e) {
        emit statusReceived(false, QStringLiteral("SUB connect failed: %1").arg(e.what()));
        emit finished();
        return;
    }

    zmq::pollitem_t items[] = {{static_cast<void *>(sub), 0, ZMQ_POLLIN, 0}};
    while (!m_stop.load()) {
        // 100 ms timeout keeps the loop responsive to requestStop().
        zmq::poll(items, 1, std::chrono::milliseconds(100));
        if (items[0].revents & ZMQ_POLLIN) {
            zmq::message_t msg;
            auto res = sub.recv(msg, zmq::recv_flags::dontwait);
            if (res) {
                dispatch(QByteArray(static_cast<const char *>(msg.data()),
                                    static_cast<int>(msg.size())));
            }
        }
    }

    sub.close();
    emit finished();
}

void TelemetryWorker::dispatch(const QByteArray &payload) {
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QJsonObject obj = doc.object();
    const QString type = obj.value(QStringLiteral("type")).toString();

    if (type == QLatin1String(proto::kObservation)) {
        const QVariantMap state = obj.value(QStringLiteral("state")).toObject().toVariantMap();
        const bool connected = obj.value(QStringLiteral("connected")).toBool(true);
        emit observationReceived(state, connected);

    } else if (type == QLatin1String(proto::kFeatures)) {
        QStringList names;
        for (const QJsonValue &v : obj.value(QStringLiteral("joints")).toArray())
            names << v.toString();

        QVariantList mins, maxs;
        const QJsonObject ranges = obj.value(QStringLiteral("ranges")).toObject();
        for (const QString &n : names) {
            const QJsonArray r = ranges.value(n).toArray();
            mins << (r.size() > 0 ? r.at(0).toDouble(-100.0) : -100.0);
            maxs << (r.size() > 1 ? r.at(1).toDouble(100.0) : 100.0);
        }
        emit featuresReceived(names, mins, maxs);

    } else if (type == QLatin1String(proto::kStatus)) {
        emit statusReceived(obj.value(QStringLiteral("connected")).toBool(false),
                            obj.value(QStringLiteral("message")).toString());
    }
}
