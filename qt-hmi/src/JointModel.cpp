#include "JointModel.h"

JointModel::JointModel(QObject *parent) : QAbstractListModel(parent) {}

int JointModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_names.size();
}

QVariant JointModel::data(const QModelIndex &index, int role) const {
    const int r = index.row();
    if (r < 0 || r >= m_names.size()) return {};
    switch (role) {
        case NameRole:   return m_names.at(r);
        case ValueRole:  return m_values.at(r);
        case TargetRole: return m_targets.at(r);
        case MinRole:    return m_mins.at(r);
        case MaxRole:    return m_maxs.at(r);
        default:         return {};
    }
}

QHash<int, QByteArray> JointModel::roleNames() const {
    return {
        {NameRole, "name"},
        {ValueRole, "value"},
        {TargetRole, "target"},
        {MinRole, "min"},
        {MaxRole, "max"},
    };
}

int JointModel::indexOf(const QString &name) const {
    return m_names.indexOf(name);
}

void JointModel::setFeatures(const QStringList &names,
                             const QList<double> &mins,
                             const QList<double> &maxs) {
    if (names == m_names) {
        // Same joints: refresh bounds in place without resetting the model.
        for (int i = 0; i < names.size(); ++i) {
            if (i < mins.size()) m_mins[i] = mins.at(i);
            if (i < maxs.size()) m_maxs[i] = maxs.at(i);
        }
        if (!names.isEmpty())
            emit dataChanged(index(0), index(names.size() - 1), {MinRole, MaxRole});
        return;
    }

    beginResetModel();
    m_names = names;
    m_mins = mins;
    m_maxs = maxs;
    m_values = QList<double>(names.size(), 0.0);
    m_targets = QList<double>(names.size(), 0.0);
    while (m_mins.size() < names.size()) m_mins.append(-100.0);
    while (m_maxs.size() < names.size()) m_maxs.append(100.0);
    endResetModel();
}

void JointModel::updateValues(const QVariantMap &state) {
    for (auto it = state.constBegin(); it != state.constEnd(); ++it) {
        const int i = indexOf(it.key());
        if (i < 0) continue;
        m_values[i] = it.value().toDouble();
        emit dataChanged(index(i), index(i), {ValueRole});
    }
}

void JointModel::setTargetByName(const QString &name, double value) {
    const int i = indexOf(name);
    if (i < 0) return;
    if (m_targets.at(i) == value) return;
    m_targets[i] = value;
    emit dataChanged(index(i), index(i), {TargetRole});
}

QVariantMap JointModel::targets() const {
    QVariantMap out;
    for (int i = 0; i < m_names.size(); ++i)
        out.insert(m_names.at(i), m_targets.at(i));
    return out;
}
