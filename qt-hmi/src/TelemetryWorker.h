#pragma once

#include <atomic>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <zmq.hpp>

// Runs the telemetry SUB socket. It lives in its own QThread: the socket is
// created and used only on that thread (ZeroMQ sockets are not thread-safe),
// and results are handed to the GUI thread through queued signal/slot delivery.
//
// The shared zmq::context_t is owned by RobotBridge and passed in by pointer;
// contexts are safe to share across threads.
class TelemetryWorker : public QObject {
    Q_OBJECT
public:
    explicit TelemetryWorker(zmq::context_t *ctx, QObject *parent = nullptr);

    void configure(const QString &endpoint);
    void requestStop() { m_stop.store(true); }

public slots:
    // Invoked once after the worker is moved onto its thread and the thread
    // starts. Blocks in a poll loop until requestStop() is called.
    void run();

signals:
    void observationReceived(const QVariantMap &state, bool connected);
    void featuresReceived(const QStringList &names,
                          const QVariantList &mins,
                          const QVariantList &maxs);
    void statusReceived(bool connected, const QString &message);
    void finished();

private:
    void dispatch(const QByteArray &payload);

    zmq::context_t *m_ctx;
    QString m_endpoint;
    std::atomic<bool> m_stop{false};
};
