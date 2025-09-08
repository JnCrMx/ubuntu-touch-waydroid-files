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

#include <QAbstractListModel>
#include <QMimeType>
#include <QObject>

#include <QCoro/QCoroQmlTask>

#include "adb_client.h"

class ADBFolderModel : public QAbstractListModel {
    Q_OBJECT

    enum Roles {
        FileNameRole = Qt::UserRole,
        StylizedFileNameRole,
        IconSourceRole,
        IconNameRole,
        FilePathRole,
        FilePathFullRole,
        MimeTypeRole,
        ModifiedDateRole,
        FileSizeRole,
        IsBrowsableRole,
        IsReadableRole,
        IsWritableRole,
        IsExecutableRole,
        FileTypeRole,
    };

public:
    ADBFolderModel();
    ~ADBFolderModel() = default;

    Q_PROPERTY(ADBClient* adbClient MEMBER m_adbClient)
    Q_PROPERTY(QString basePath MEMBER m_basePath NOTIFY basePathChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)

    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY currentPathChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY currentPathChanged)

    Q_INVOKABLE QCoro::QmlTask goTo(const QString& path);
    Q_INVOKABLE QCoro::QmlTask goBack();
    Q_INVOKABLE QCoro::QmlTask goForward();

    const QString& currentPath() const { return m_currentPath; }

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    QMimeType mimeType(const ADBFileEntry& entry) const;
    QString iconName(const ADBFileEntry& entry) const;
    QString fileSize(qint64 size) const;

    bool canGoBack() const { return m_historyIndex > 0; }
    bool canGoForward() const { return m_historyIndex < (m_history.size()-1); }
signals:
    void currentPathChanged();
    void basePathChanged();

private:
    ADBClient* m_adbClient;
    QString m_basePath = "/";

    QString m_currentPath = "";
    std::vector<ADBFileEntry> m_entries{};

    QStringList m_history{};
    int m_historyIndex = -1;

    QCoro::Task<void> updateFolder();
};

#endif
