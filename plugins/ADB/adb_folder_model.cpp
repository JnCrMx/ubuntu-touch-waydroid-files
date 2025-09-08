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
#include "adb_folder_model.h"

#include <filesystem>

#include <QDateTime>
#include <QDebug>
#include <QMimeDatabase>
#include <QMimeType>

#include <sys/stat.h>

ADBFolderModel::ADBFolderModel() {
    connect(this, &ADBFolderModel::basePathChanged, this, [this]() {
        m_history.clear();
        m_historyIndex = -1;

        m_currentPath.clear();
        emit currentPathChanged();
    });
    connect(this, &ADBFolderModel::currentPathChanged, this, [this]() {
        m_selectedFile.clear();
        emit selectedFileChanged();
    });
    connect(this, &ADBFolderModel::selectedFileChanged, this, [this]() {
        qDebug() << "Selected file changed to" << m_selectedFile;
        emit dataChanged(index(0, 0), index(rowCount({}) - 1, 0), {Roles::IsSelectedRole});
    });
}

QHash<int, QByteArray> ADBFolderModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Roles::FileNameRole] = "fileName";
    roles[Roles::StylizedFileNameRole] = "stylizedFileName";
    roles[Roles::IconSourceRole] = "iconSource";
    roles[Roles::IconNameRole] = "iconName";
    roles[Roles::FilePathRole] = "filePath";
    roles[Roles::FilePathFullRole] = "filePathFull";
    roles[Roles::MimeTypeRole] = "mimeType";
    roles[Roles::ModifiedDateRole] = "modifiedDate";
    roles[Roles::FileSizeRole] = "fileSize";
    roles[Roles::IsBrowsableRole] = "isBrowsable";
    roles[Roles::IsReadableRole] = "isReadable";
    roles[Roles::IsWritableRole] = "isWritable";
    roles[Roles::IsExecutableRole] = "isExecutable";
    roles[Roles::FileTypeRole] = "fileType";
    roles[Roles::IsSelectedRole] = "isSelected";
    return roles;
}
int ADBFolderModel::rowCount(const QModelIndex& parent) const {
    if(parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_entries.size());
}
QVariant ADBFolderModel::data(const QModelIndex& index, int role) const {
    if(!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size())) {
        return {};
    }

    const ADBFileEntry& entry = m_entries.at(static_cast<size_t>(index.row()));
    bool is_dir = S_ISDIR(entry.mode);
    bool is_regular = S_ISREG(entry.mode);
    bool is_link = S_ISLNK(entry.mode);

    switch(role) {
        case Roles::FileNameRole:
            return entry.fileName;
        case Roles::StylizedFileNameRole:
            return entry.fileName;
        case Roles::IconSourceRole:
            return is_dir ? QLatin1String("image://theme/icon-m-common-directory") : QLatin1String("image://theme/icon-m-content-document");
        case Roles::IconNameRole:
            return iconName(entry);
        case Roles::FilePathRole:
            return (m_currentPath.isEmpty() || m_currentPath.endsWith("/")) ? m_currentPath + entry.fileName : m_currentPath + "/" + entry.fileName;
        case Roles::FilePathFullRole: {
            std::filesystem::path p = m_basePath.toStdString();
            p /= m_currentPath.toStdString();
            p /= entry.fileName.toStdString();
            p = std::filesystem::weakly_canonical(p);
            return QString::fromStdString(p.string());
        }
        case Roles::MimeTypeRole: {
            auto type = mimeType(entry);
            return type.isValid() ? type.name() : QString{};
        }
        case Roles::ModifiedDateRole:
            return QDateTime::fromSecsSinceEpoch(entry.time);
        case Roles::FileSizeRole:
            return is_regular ? fileSize(entry.size) : QString{};
        case Roles::IsBrowsableRole:
            return is_dir;
        case Roles::IsReadableRole:
            return (entry.mode & S_IRUSR) || (entry.mode & S_IRGRP) || (entry.mode & S_IROTH);
        case Roles::IsWritableRole:
            return (entry.mode & S_IWUSR) || (entry.mode & S_IWGRP) || (entry.mode & S_IWOTH);
        case Roles::IsExecutableRole:
            return (entry.mode & S_IXUSR) || (entry.mode & S_IXGRP) || (entry.mode & S_IXOTH);
        case Roles::FileTypeRole:
            if(is_dir) {
                return "directory";
            } else if(is_regular) {
                return "file";
            } else if(is_link) {
                return "symlink";
            } else {
                return "other";
            }
        case Roles::IsSelectedRole:
            return m_selectedFile == data(index, Roles::FilePathFullRole).toString();
        default:
            return {};
    }
}

QMimeType ADBFolderModel::mimeType(const ADBFileEntry& entry) const {
    static QMimeDatabase mimeDb;

    auto types = mimeDb.mimeTypesForFileName(entry.fileName);

    if(!types.isEmpty()) {
        return types.first();
    } else {
        return QMimeType{};
    }
}

QString ADBFolderModel::iconName(const ADBFileEntry& entry) const {
    bool is_dir = S_ISDIR(entry.mode);
    bool is_link = S_ISLNK(entry.mode);
    bool is_regular = S_ISREG(entry.mode);

    auto type = mimeType(entry);
    if(is_dir) {
        return "folder";
    } else if(is_link) {
        return "emblem-symbolic-link";
    } else if(type.isValid()) {
        return type.iconName();
    } else if(is_regular) {
        return "text-x-generic";
    } else {
        return "unknown";
    }
}

QString ADBFolderModel::fileSize(qint64 size) const
{
    struct UnitSizes {
        qint64      bytes;
        const char *name;
    };

    constexpr UnitSizes m_unitBytes[5] = {
        { 1,           "Bytes" }
        , {1024,         "KiB"}
        , {1024 * 1024,  "MiB"}
        , {1024 *  m_unitBytes[2].bytes,   "GiB"}
        , {1024 *  m_unitBytes[3].bytes, "TiB"}
    };

    QString ret;
    int unit = sizeof(m_unitBytes) / sizeof(m_unitBytes[0]);
    while ( unit-- > 1 && size < m_unitBytes[unit].bytes );

    if (unit > 0 ) {
        ret.sprintf("%0.1f %s", (float)size / m_unitBytes[unit].bytes,
                    m_unitBytes[unit].name);

    } else {
        ret.sprintf("%ld %s", (long int)size, m_unitBytes[0].name);
    }

    return ret;
}

QCoro::QmlTask ADBFolderModel::goTo(const QString& path) {
    if(path != m_currentPath) {
        if(m_historyIndex < (int)m_history.size() - 1) {
            m_history.erase(m_history.begin() + m_historyIndex + 1, m_history.end());
        }
        m_history.push_back(path);
        m_historyIndex++;

        m_currentPath = path;
        emit currentPathChanged();
    }
    return updateFolder();
}

QCoro::Task<void> nothing() {
    co_return;
}

QCoro::QmlTask ADBFolderModel::goBack() {
    if(canGoBack()) {
        m_historyIndex--;
        m_currentPath = m_history.at(m_historyIndex);
        emit currentPathChanged();

        return updateFolder();
    }
    return nothing();
}
QCoro::QmlTask ADBFolderModel::goForward() {
    if(canGoForward()) {
        m_historyIndex++;
        m_currentPath = m_history.at(m_historyIndex);
        emit currentPathChanged();

        return updateFolder();
    }
    return nothing();
}

QCoro::Task<void> ADBFolderModel::updateFolder() {
    if(!m_adbClient) {
        co_return;
    }

    beginResetModel();
    m_entries = co_await m_adbClient->co_listFiles(m_basePath + "/" + m_currentPath);
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [](const ADBFileEntry& entry) -> bool {
        return entry.fileName == "." || entry.fileName == "..";
    }), m_entries.end());
    std::sort(m_entries.begin(), m_entries.end(), [](const ADBFileEntry& a, const ADBFileEntry& b) -> bool {
        bool a_is_dir = S_ISDIR(a.mode);
        bool b_is_dir = S_ISDIR(b.mode);
        if(a_is_dir != b_is_dir) {
            return a_is_dir > b_is_dir;
        }
        bool a_is_regular = S_ISREG(a.mode);
        bool b_is_regular = S_ISREG(b.mode);
        if(a_is_regular != b_is_regular) {
            return a_is_regular > b_is_regular;
        }
        return a.fileName.toLower() < b.fileName.toLower();
    });

    endResetModel();
    co_return;
}
