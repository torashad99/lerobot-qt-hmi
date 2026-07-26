#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QThread>

#include <zmq.hpp>

#include "JointModel.h"
#include "TelemetryWorker.h"

// The single object QML talks to. It lives on the GUI thread and:
//   - owns the outbound action PUSH socket (used only on the GUI thread),
//   - owns a worker thread running the inbound telemetry SUB socket,
//   - owns the JointModel that backs the UI.
//
// One shared zmq::context_t is used for both sockets. The PUSH socket is only
// ever touched here (GUI thread); the SUB socket is only ever touched by the
// worker. That split keeps each non-thread-safe socket pinned to one thread.
class RobotBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(int actionPort READ actionPort WRITE setActionPort NOTIFY actionPortChanged)
    Q_PROPERTY(int telemetryPort READ telemetryPort WRITE setTelemetryPort NOTIFY telemetryPortChanged)
    Q_PROPERTY(QAbstractItemModel *jointModel READ jointModel CONSTANT)

public:
    explicit RobotBridge(QObject *parent = nullptr);
    ~RobotBridge() override;

    bool connected() const { return m_connected; }
    QString statusMessage() const { return m_statusMessage; }
    QString host() const { return m_host; }
    int actionPort() const { return m_actionPort; }
    int telemetryPort() const { return m_telemetryPort; }
    QAbstractItemModel *jointModel() { return &m_model; }

    void setHost(const QString &host);
    void setActionPort(int port);
    void setTelemetryPort(int port);

public slots:
    // Open the action socket and start the telemetry worker.
    void start();
    // Stop the worker and close sockets.
    void stop();

    // Called by a slider: update the model target and send the full goal vector.
    void setJointTarget(const QString &name, double value);

    // Control-plane verbs: "home", "estop", "enable", "disable".
    void sendCommand(const QString &verb);

signals:
    void connectedChanged();
    void statusMessageChanged();
    void hostChanged();
    void actionPortChanged();
    void telemetryPortChanged();

private slots:
    void onObservation(const QVariantMap &state, bool connected);
    void onFeatures(const QStringList &names, const QVariantList &mins, const QVariantList &maxs);
    void onStatus(bool connected, const QString &message);

private:
    void sendJson(const QJsonObject &obj);
    void setConnected(bool c);
    void setStatusMessage(const QString &m);
    QString actionEndpoint() const;
    QString telemetryEndpoint() const;

    zmq::context_t m_ctx{1};
    zmq::socket_t m_push;  // action channel, GUI thread only
    bool m_pushOpen = false;

    QThread m_workerThread;
    TelemetryWorker *m_worker = nullptr;

    JointModel m_model;

    QString m_host = QStringLiteral("localhost");
    int m_actionPort = 5556;
    int m_telemetryPort = 5557;
    bool m_connected = false;
    QString m_statusMessage = QStringLiteral("idle");
    quint64 m_seq = 0;
};
