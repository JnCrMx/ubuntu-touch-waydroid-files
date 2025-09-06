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
#include <QDataStream>

#include "qcoro/qcorotask.h"

class QTimer;

class ADBClient : public QObject {
    Q_OBJECT

public:
    ADBClient();
    ~ADBClient() = default;

    Q_PROPERTY(int probeInterval MEMBER m_probeInterval)
signals:
    void deviceFound();
private:
    QTimer* m_probeTimer = nullptr;
    int m_probeInterval = 1000;

    QCoro::Task<void> probe();
};

#endif
