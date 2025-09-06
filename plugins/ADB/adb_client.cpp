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

#include <QDebug>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <expected>
#include "qcoroabstractsocket.h"

#include "adb_client.h"

ADBClient::ADBClient() {
    m_probeTimer = new QTimer(this);
    m_probeTimer->setInterval(m_probeInterval);
    connect(m_probeTimer, &QTimer::timeout, this, [this]() {
        probe();
    });
    m_probeTimer->start();
}

enum class ADBProtolError {
    InvalidStatus,
    InvalidLength,
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

    QByteArray len = co_await co_socket.read(4);
    bool okay{};
    int l = len.toInt(&okay, 16);
    if(!okay) {
        co_return std::unexpected(ADBProtolError::InvalidLength);
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

QCoro::Task<void> ADBClient::probe() {
    QTcpSocket socket;
    auto co_socket = qCoro(socket);

    bool okay = co_await co_socket.connectToHost(QHostAddress::LocalHost, 5037);
    if(!okay) {
        qDebug() << "Failed to connect to ADB server";
        co_return;
    }
    QByteArray res = (co_await sendRequest(socket, "host:devices")).value();

    if(!res.isEmpty()) {
        emit deviceFound();
        m_probeTimer->stop();
    }

    co_return;
}
