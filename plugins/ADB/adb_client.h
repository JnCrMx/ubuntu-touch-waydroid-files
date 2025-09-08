/*
 * Copyright (C) 2025  JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * waydroid-files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ADB_CLIENT_H
#define ADB_CLIENT_H

#include <QObject>
#include <QUrl>

#include <QCoro/QCoroCore>
#include <QCoro/QCoroQmlTask>

class QTimer;

struct ADBFileEntry {
    QString fileName;
    uint32_t mode;
    uint32_t size;
    uint32_t time;
};

class ADBClient : public QObject {
    Q_OBJECT

public:
    ADBClient();
    ~ADBClient() = default;

    Q_PROPERTY(int probeInterval MEMBER m_probeInterval)

    QCoro::Task<std::optional<ADBFileEntry>> co_stat(QString path);
    QCoro::Task<std::vector<ADBFileEntry>> co_listFiles(QString path);

    QCoro::Task<QString> co_findFirstAccessible(QStringList paths);
    QCoro::Task<QString> co_findFirstAccessibleFolder(QStringList paths);
    QCoro::Task<QString> co_findFirstAccessibleRegularFile(QStringList paths);
    QCoro::Task<QUrl> co_pullFile(QString path);
    QCoro::Task<bool> co_pushFile(QString hostPath, QString devicePath, mode_t mode = 0644);
    QCoro::Task<bool> co_pushFileFromUrl(QUrl hostUrl, QString devicePath, mode_t mode = 0644);

    // Q_INVOKABLE QCoro::QmlTask stat(const QString& path) {
    //     return co_stat(path);
    // }
    // Q_INVOKABLE QCoro::QmlTask listFiles(const QString& path) {
    //     return co_listFiles(path);
    // }
    Q_INVOKABLE QCoro::QmlTask findFirstAccessible(const QStringList& paths) {
        return co_findFirstAccessible(paths);
    }
    Q_INVOKABLE QCoro::QmlTask findFirstAccessibleFolder(const QStringList& paths) {
        return co_findFirstAccessibleFolder(paths);
    }
    Q_INVOKABLE QCoro::QmlTask findFirstAccessibleRegularFile(const QStringList& paths) {
        return co_findFirstAccessibleRegularFile(paths);
    }
    Q_INVOKABLE QCoro::QmlTask pullFile(const QString& path) {
        return co_pullFile(path);
    }
    Q_INVOKABLE QCoro::QmlTask pushFile(const QString& hostPath, const QString& devicePath, int mode = 0644) {
        return co_pushFile(hostPath, devicePath, mode);
    }
    Q_INVOKABLE QCoro::QmlTask pushFileFromUrl(const QUrl& hostUrl, const QString& devicePath, int mode = 0644) {
        return co_pushFileFromUrl(hostUrl, devicePath, mode);
    }
    Q_INVOKABLE void cleanupPulledFiles();
signals:
    void deviceFound();
private:
    QTimer* m_probeTimer = nullptr;
    int m_probeInterval = 1000;

    QCoro::Task<void> co_probe();
};

#endif
