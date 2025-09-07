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
#include "adb_client.h"

#include <expected>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>

#include <QCoro/QCoroAbstractSocket>

#include <arpa/inet.h>
#include <sys/stat.h>

ADBClient::ADBClient() {
    m_probeTimer = new QTimer(this);
    m_probeTimer->setInterval(m_probeInterval);
    connect(m_probeTimer, &QTimer::timeout, this, [this]() {
        co_probe();
    });
    m_probeTimer->start();
}

enum class ADBProtolError {
    InvalidStatus,
    TruncatedPayload,
};

using ADBError = std::variant<QByteArray, ADBProtolError>;
using ADBResult = std::expected<QByteArray, ADBError>;

QCoro::Task<std::expected<QByteArray, ADBError>> sendRequest(QTcpSocket& socket, const QByteArray& req) {
    QByteArray r = QString::number(req.size(), 16).rightJustified(4, '0').toUtf8() + req;

    auto co_socket = qCoro(socket);
    co_await co_socket.write(r);

    QByteArray status = co_await co_socket.read(4);
    if(status != "OKAY" && status != "FAIL") {
        co_return std::unexpected(ADBProtolError::InvalidStatus);
    }

    QByteArray len = co_await co_socket.read(4, std::chrono::milliseconds{1});
    bool okay{};
    int l = len.toInt(&okay, 16);
    if(!okay) {
        if(status == "FAIL") {
            co_return std::unexpected(QByteArray(""));
        }
        co_return QByteArray{};
    }

    r = co_await co_socket.read(l);
    if(r.size() != l) {
        co_return std::unexpected(ADBProtolError::TruncatedPayload);
    }

    if(status == "FAIL") {
        co_return std::unexpected(r);
    }
    co_return r;
}

QCoro::Task<void> ADBClient::co_probe() {
    QTcpSocket socket;
    auto co_socket = qCoro(socket);

    bool okay = co_await co_socket.connectToHost(QHostAddress::LocalHost, 5037);
    if(!okay) {
        qDebug() << "Failed to connect to ADB server";
        co_return;
    }
    auto res = (co_await sendRequest(socket, "host:get-state"));
    if(!res) {
        co_return;
    }

    if(*res == "device") {
        emit deviceFound();
        m_probeTimer->stop();
    }

    co_return;
}

struct [[gnu::packed]] sync_dent_rest {
    uint32_t mode;
    uint32_t size;
    uint32_t time;
    uint32_t namelen;
};
struct [[gnu::packed]] sync_stat_rest {
    uint32_t mode;
    uint32_t size;
    uint32_t time;
};
struct [[gnu::packed]] sync_data_rest {
    uint32_t size;
};

QCoro::Task<std::vector<ADBFileEntry>> ADBClient::co_listFiles(QString path) {
    if(!path.endsWith('/')) {
        path += '/';
    }

    std::vector<ADBFileEntry> entries;

    QTcpSocket socket;
    auto co_socket = qCoro(socket);

    bool okay = co_await co_socket.connectToHost(QHostAddress::LocalHost, 5037);
    if(!okay) {
        qWarning() << "Failed to connect to ADB server";
        co_return entries;
    }

    if(!(co_await sendRequest(socket, "host:transport-any"))) {
        co_return entries;
    }

    if(!(co_await sendRequest(socket, "sync:"))) {
        co_return entries;
    }

    QByteArray rawPath = path.toUtf8();
    uint32_t rawPathLen = rawPath.size();
    QByteArray syncRequest = "LIST" + QByteArray::fromRawData(reinterpret_cast<const char*>(&rawPathLen), sizeof(uint32_t)) + rawPath;

    co_await co_socket.write(syncRequest);

    while(true) {
        QByteArray status = co_await co_socket.read(4);
        if(status == "FAIL") {
            QByteArray len = co_await co_socket.read(4);
            if(len.size() != 4) {
                qWarning() << "Protocol error, message length truncated";
                co_return entries;
            }
            uint32_t l = *reinterpret_cast<const uint32_t*>(len.constData());
            QByteArray msg = co_await co_socket.read(l);
            qWarning() << "ADB error:" << QString::fromUtf8(msg);
            co_return entries;
        } else if(status == "DONE") {
            QByteArray unused = co_await co_socket.read(sizeof(sync_dent_rest));
            break;
        } else if(status == "DENT") {
            QByteArray dent = co_await co_socket.read(sizeof(sync_dent_rest));
            if(dent.size() != sizeof(sync_dent_rest)) {
                qWarning() << "Protocol error, DENT truncated";
                co_return entries;
            }
            const sync_dent_rest* dent_rest = reinterpret_cast<const sync_dent_rest*>(dent.constData());
            uint32_t namelen = dent_rest->namelen;
            QByteArray name = co_await co_socket.read(namelen);
            if(name.size() != static_cast<int>(namelen)) {
                qWarning() << "Protocol error, name truncated";
                co_return entries;
            }

            ADBFileEntry entry;
            entry.fileName = QString::fromUtf8(name);
            entry.mode = dent_rest->mode;
            entry.size = dent_rest->size;
            entry.time = dent_rest->time;
            entries.push_back(entry);
        } else {
            qWarning() << "Protocol error, invalid status" << status;
            co_return entries;
        }
    }

    co_return entries;
}

QCoro::Task<std::optional<ADBFileEntry>> ADBClient::co_stat(QString path) {
    QTcpSocket socket;
    auto co_socket = qCoro(socket);

    bool okay = co_await co_socket.connectToHost(QHostAddress::LocalHost, 5037);
    if(!okay) {
        qWarning() << "Failed to connect to ADB server";
        co_return std::nullopt;
    }

    if(!(co_await sendRequest(socket, "host:transport-any"))) {
        co_return std::nullopt;
    }

    if(!(co_await sendRequest(socket, "sync:"))) {
        co_return std::nullopt;
    }

    QByteArray rawPath = path.toUtf8();
    uint32_t rawPathLen = rawPath.size();
    QByteArray syncRequest = "STAT" + QByteArray::fromRawData(reinterpret_cast<const char*>(&rawPathLen), sizeof(uint32_t)) + rawPath;

    co_await co_socket.write(syncRequest);

    QByteArray status = co_await co_socket.read(4);
    if(status == "FAIL") {
        QByteArray len = co_await co_socket.read(4);
        if(len.size() != 4) {
            qWarning() << "Protocol error, message length truncated";
            co_return std::nullopt;
        }
        uint32_t l = *reinterpret_cast<const uint32_t*>(len.constData());
        QByteArray msg = co_await co_socket.read(l);
        qWarning() << "ADB error:" << QString::fromUtf8(msg);
        co_return std::nullopt;
    } else if(status == "STAT") {
        QByteArray dent = co_await co_socket.read(sizeof(sync_stat_rest));
        if(dent.size() != sizeof(sync_stat_rest)) {
            qWarning() << "Protocol error, STAT truncated";
            co_return std::nullopt;
        }
        const sync_stat_rest* dent_rest = reinterpret_cast<const sync_stat_rest*>(dent.constData());

        ADBFileEntry entry;
        entry.fileName = QString::fromUtf8(path.toUtf8().split('/').last());
        entry.mode = dent_rest->mode;
        entry.size = dent_rest->size;
        entry.time = dent_rest->time;
        co_return entry;
    } else {
        qWarning() << "Protocol error, invalid status" << status;
        co_return std::nullopt;
    }

    co_return std::nullopt;
}

QCoro::Task<QUrl> ADBClient::co_pullFile(QString path) {
    QTcpSocket socket;
    auto co_socket = qCoro(socket);

    bool okay = co_await co_socket.connectToHost(QHostAddress::LocalHost, 5037);
    if(!okay) {
        qWarning() << "Failed to connect to ADB server";
        co_return {};
    }

    if(!(co_await sendRequest(socket, "host:transport-any"))) {
        co_return {};
    }

    if(!(co_await sendRequest(socket, "sync:"))) {
        co_return {};
    }

    QByteArray rawPath = path.toUtf8();
    uint32_t rawPathLen = rawPath.size();
    QByteArray syncRequest = "RECV" + QByteArray::fromRawData(reinterpret_cast<const char*>(&rawPathLen), sizeof(uint32_t)) + rawPath;

    co_await co_socket.write(syncRequest);

    QString destinationFolder = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/PulledFiles";
    if(!QDir{destinationFolder}.mkpath(".")) {
        qWarning() << "Failed to create destination folder:" << destinationFolder;
        co_return {};
    }
    QFile file{destinationFolder + "/" + path.split('/').last()};
    if(!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << file.fileName();
        co_return {};
    }

    while(true) {
        QByteArray status = co_await co_socket.read(4);
        if(status == "FAIL") {
            file.close();
            file.remove();

            QByteArray len = co_await co_socket.read(4);
            if(len.size() != 4) {
                qWarning() << "Protocol error, message length truncated";
                co_return {};
            }
            uint32_t l = *reinterpret_cast<const uint32_t*>(len.constData());
            QByteArray msg = co_await co_socket.read(l);
            qWarning() << "ADB error:" << QString::fromUtf8(msg);
            co_return {};
        } else if(status == "DONE") {
            QByteArray unused = co_await co_socket.read(sizeof(sync_data_rest));
            break;
        } else if(status == "DATA") {
            QByteArray data = co_await co_socket.read(sizeof(sync_data_rest));
            if(data.size() != sizeof(sync_data_rest)) {
                qWarning() << "Protocol error, DATA truncated";

                file.close();
                file.remove();
                co_return {};
            }
            const sync_data_rest* data_rest = reinterpret_cast<const sync_data_rest*>(data.constData());
            uint32_t size = data_rest->size;
            QByteArray filedata{};
            do {
                filedata += co_await co_socket.read(size - filedata.size());
            } while(filedata.size() < static_cast<int>(size));
            if(filedata.size() != static_cast<int>(size)) {
                qWarning() << "Protocol error, DATA payload wrong size, expected" << size << "got" << filedata.size();

                file.close();
                file.remove();
                co_return {};
            }

            file.write(filedata);
        } else {
            qWarning() << "Protocol error, invalid status" << status;

            file.close();
            file.remove();
        }
    }
    file.close();

    co_return QUrl::fromLocalFile(file.fileName());
}

void ADBClient::cleanupPulledFiles() {
    QString destinationFolder = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/PulledFiles";
    QDir dir{destinationFolder};
    if(dir.exists()) {
        dir.removeRecursively();
    }
}

QCoro::Task<QString> ADBClient::co_findFirstAccessible(QStringList paths) {
    for(const QString& path : paths) {
        auto entry = co_await co_stat(path);
        if(entry) {
            co_return path;
        }
    }
    co_return QString();
}
QCoro::Task<QString> ADBClient::co_findFirstAccessibleFolder(QStringList paths) {
    for(const QString& path : paths) {
        auto entry = co_await co_stat(path);
        if(entry && S_ISDIR(entry->mode)) {
            co_return path;
        }
    }
    co_return QString();
}
QCoro::Task<QString> ADBClient::co_findFirstAccessibleRegularFile(QStringList paths) {
    for(const QString& path : paths) {
        auto entry = co_await co_stat(path);
        if(entry && S_ISREG(entry->mode)) {
            co_return path;
        }
    }
    co_return QString();
}
