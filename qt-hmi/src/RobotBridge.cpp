#include "RobotBridge.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "Protocol.h"

RobotBridge::RobotBridge(QObject *parent)
    : QObject(parent), m_push(m_ctx, zmq::socket_type::push) {}

RobotBridge::~RobotBridge() {
    stop();
}

QString RobotBridge::actionEndpoint() const {
    return QStringLiteral("tcp://%1:%2").arg(m_host).arg(m_actionPort);
}

QString RobotBridge::telemetryEndpoint() const {
    return QStringLiteral("tcp://%1:%2").arg(m_host).arg(m_telemetryPort);
}

void RobotBridge::setHost(const QString &host) {
    if (host == m_host) return;
    m_host = host;
    emit hostChanged();
}

void RobotBridge::setActionPort(int port) {
    if (port == m_actionPort) return;
    m_actionPort = port;
    emit actionPortChanged();
}

void RobotBridge::setTelemetryPort(int port) {
    if (port == m_telemetryPort) return;
    m_telemetryPort = port;
    emit telemetryPortChanged();
}

void RobotBridge::setConnected(bool c) {
    if (c == m_connected) return;
    m_connected = c;
    emit connectedChanged();
}

void RobotBridge::setStatusMessage(const QString &m) {
    if (m == m_statusMessage) return;
    m_statusMessage = m;
    emit statusMessageChanged();
}

void RobotBridge::start() {
    stop();

    // Outbound action socket. A small send high-water mark plus non-blocking
    // sends give latest-wins behaviour and never stall the UI.
    m_push = zmq::socket_t(m_ctx, zmq::socket_type::push);
    m_push.set(zmq::sockopt::sndhwm, 8);
    m_push.set(zmq::sockopt::linger, 0);
    try {
        m_push.connect(actionEndpoint().toStdString());
        m_pushOpen = true;
    } catch (const zmq::error_t &e) {
        setStatusMessage(QStringLiteral("PUSH connect failed: %1").arg(e.what()));
        return;
    }

    // Inbound telemetry worker on its own thread.
    m_worker = new TelemetryWorker(&m_ctx);
    m_worker->configure(telemetryEndpoint());
    m_worker->moveToThread(&m_workerThread);

    connect(&m_workerThread, &QThread::started, m_worker, &TelemetryWorker::run);
    connect(m_worker, &TelemetryWorker::finished, &m_workerThread, &QThread::quit);
    connect(m_worker, &TelemetryWorker::finished, m_worker, &QObject::deleteLater);
    connect(m_worker, &TelemetryWorker::observationReceived, this, &RobotBridge::onObservation);
    connect(m_worker, &TelemetryWorker::featuresReceived, this, &RobotBridge::onFeatures);
    connect(m_worker, &TelemetryWorker::statusReceived, this, &RobotBridge::onStatus);

    m_workerThread.start();
    setStatusMessage(QStringLiteral("started; waiting for telemetry"));
}

void RobotBridge::stop() {
    if (m_worker) {
        m_worker->requestStop();
    }
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        m_workerThread.wait(1000);
    }
    m_worker = nullptr;

    if (m_pushOpen) {
        m_push.close();
        m_pushOpen = false;
    }
    setConnected(false);
}

void RobotBridge::sendJson(const QJsonObject &obj) {
    if (!m_pushOpen) return;
    QJsonObject withMeta = obj;
    withMeta.insert(QStringLiteral("v"), proto::kVersion);
    withMeta.insert(QStringLiteral("seq"), static_cast<double>(m_seq++));
    withMeta.insert(QStringLiteral("ts"), QDateTime::currentMSecsSinceEpoch() / 1000.0);
    const QByteArray bytes = QJsonDocument(withMeta).toJson(QJsonDocument::Compact);
    try {
        m_push.send(zmq::buffer(bytes.constData(), bytes.size()), zmq::send_flags::dontwait);
    } catch (const zmq::error_t &) {
        // EAGAIN under back-pressure: drop this frame, the next one supersedes it.
    }
}

void RobotBridge::setJointTarget(const QString &name, double value) {
    m_model.setTargetByName(name, value);

    // Send the full goal vector so the service always has a complete action.
    const QVariantMap targets = m_model.targets();
    QJsonObject action;
    for (auto it = targets.constBegin(); it != targets.constEnd(); ++it)
        action.insert(it.key(), it.value().toDouble());

    QJsonObject msg;
    msg.insert(QStringLiteral("type"), QLatin1String(proto::kAction));
    msg.insert(QStringLiteral("action"), action);
    sendJson(msg);
}

void RobotBridge::sendCommand(const QString &verb) {
    QJsonObject msg;
    msg.insert(QStringLiteral("type"), QLatin1String(proto::kCommand));
    msg.insert(QStringLiteral("command"), verb);
    sendJson(msg);
}

void RobotBridge::onObservation(const QVariantMap &state, bool connected) {
    m_model.updateValues(state);
    setConnected(connected);
}

void RobotBridge::onFeatures(const QStringList &names, const QVariantList &mins, const QVariantList &maxs) {
    QList<double> dmins, dmaxs;
    dmins.reserve(mins.size());
    dmaxs.reserve(maxs.size());
    for (const QVariant &v : mins) dmins << v.toDouble();
    for (const QVariant &v : maxs) dmaxs << v.toDouble();
    m_model.setFeatures(names, dmins, dmaxs);
}

void RobotBridge::onStatus(bool connected, const QString &message) {
    setConnected(connected);
    setStatusMessage(message);
}
