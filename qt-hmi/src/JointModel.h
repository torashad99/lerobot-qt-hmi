#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QVariantMap>

// Passive data holder for the joint table. RobotBridge owns it and pushes
// updates; QML binds sliders to it. The "target" role is user-owned (the
// commanded goal); the "value" role is the observed position from telemetry,
// so the two never fight.
class JointModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole,
        TargetRole,
        MinRole,
        MaxRole,
    };

    explicit JointModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Rebuild the whole table from a features frame.
    void setFeatures(const QStringList &names,
                     const QList<double> &mins,
                     const QList<double> &maxs);

    // Merge observed positions from an observation frame.
    void updateValues(const QVariantMap &state);

    // Set a commanded target by joint name (called when a slider moves).
    void setTargetByName(const QString &name, double value);

    // Snapshot of all current targets, as a name -> value map.
    QVariantMap targets() const;

    QStringList names() const { return m_names; }

private:
    int indexOf(const QString &name) const;

    QStringList m_names;
    QList<double> m_values;
    QList<double> m_targets;
    QList<double> m_mins;
    QList<double> m_maxs;
};
