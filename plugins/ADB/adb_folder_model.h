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

#ifndef ADB_FOLDER_MODEL_H
#define ADB_FOLDER_MODEL_H

#include <QObject>
#include <QAbstractListModel>
#include "adb_client.h"

class ADBFolderModel : public QAbstractListModel {
    Q_OBJECT

    enum Roles {
        FileNameRole = Qt::UserRole + 1,
        StylizedFileNameRole
    };

public:
    ADBFolderModel();
    ~ADBFolderModel() = default;

    Q_PROPERTY(ADBClient* adbClient MEMBER m_adbClient)
    Q_PROPERTY(QString basePath MEMBER m_basePath)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)

    Q_INVOKABLE void goTo(const QString& path);

    const QString& filePath() const { return m_filePath; }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles;
        roles.insert(Roles::FileNameRole, "fileName");
        roles.insert(Roles::StylizedFileNameRole, "stylizedFileName");
        return roles;
    }

    int rowCount(const QModelIndex& parent) const override {
        return 0;
    }

    QVariant data(const QModelIndex& index, int role) const override {
        return QString{"test"};
    }
signals:
    void filePathChanged();

private:
    ADBClient* m_adbClient;
    QString m_basePath = "/";

    QString m_filePath = "/";
};

#endif
