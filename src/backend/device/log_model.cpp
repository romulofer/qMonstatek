/*
 * log_model.cpp — Application log capture model
 */

#include "log_model.h"
#include <QDateTime>
#include <QThread>

static LogModel *s_globalLogModel = nullptr;

LogModel* globalLogModel() { return s_globalLogModel; }
void setGlobalLogModel(LogModel *model) { s_globalLogModel = model; }

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

LogModel::~LogModel()
{
    // The Qt message handler keeps a raw pointer to us; never leave it dangling.
    if (s_globalLogModel == this)
        s_globalLogModel = nullptr;
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    QMutexLocker lock(&m_mutex);
    return m_lines.size();
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    QMutexLocker lock(&m_mutex);
    if (!index.isValid() || index.row() >= m_lines.size())
        return {};
    if (role == MessageRole || role == Qt::DisplayRole)
        return m_lines.at(index.row());
    return {};
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {{ MessageRole, "message" }};
}

void LogModel::clear()
{
    QMutexLocker lock(&m_mutex);
    beginResetModel();
    m_lines.clear();
    endResetModel();
    emit countChanged();
}

QString LogModel::fullText() const
{
    QMutexLocker lock(&m_mutex);
    return m_lines.join('\n');
}

void LogModel::append(const QString &line)
{
    // The Qt message handler also runs on worker threads (e.g. the WiFi
    // NetworkWorker). Model changes and signals must happen on our own thread.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, line]() { append(line); },
                                  Qt::QueuedConnection);
        return;
    }
    {
        QMutexLocker lock(&m_mutex);
        if (m_lines.size() >= MAX_LINES) {
            beginRemoveRows(QModelIndex(), 0, 0);
            m_lines.removeFirst();
            endRemoveRows();
        }
        int row = m_lines.size();
        beginInsertRows(QModelIndex(), row, row);
        m_lines.append(line);
        endInsertRows();
    }
    emit countChanged();
    emit lineAppended(line);
}
