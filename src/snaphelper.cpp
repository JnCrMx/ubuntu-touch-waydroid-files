#include "snaphelper.h"

#include <QApplication>
#include <QFileDialog>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QDBusVariant>

#include <QCoro/QCoroSignal>
#include <QCoro/QCoroDBus>

#include <fcntl.h>

SnapHelper::SnapHelper() {

}

bool SnapHelper::isSnap() const {
    return std::getenv("SNAP") != nullptr;
}

QCoro::Task<bool> SnapHelper::co_saveFileDialog(QUrl file, bool show) {
    QDBusInterface fileChooser("org.freedesktop.portal.Desktop",
                         "/org/freedesktop/portal/desktop",
                          "org.freedesktop.portal.FileChooser",
                          QDBusConnection::sessionBus());

    auto co_reply = qCoro(fileChooser.asyncCall("SaveFile",
        "", "Save File", QVariantMap{
            {"current_name", file.fileName().split('/').last()}
        }));
    QDBusReply<QDBusObjectPath> reply = co_await co_reply.waitForFinished();
    QDBusObjectPath requestPath = reply.value();

    QDBusInterface request("org.freedesktop.portal.Desktop",
                         requestPath.path(),
                          "org.freedesktop.portal.Request",
                          QDBusConnection::sessionBus());
    ResponseProxy proxy{};
    connect(&request, SIGNAL(Response(uint, QVariantMap)), &proxy, SLOT(slot(uint, QVariantMap)));

    auto [response, results] = co_await qCoro(&proxy, &ResponseProxy::signal);
    if(response != 0) {
        co_return false;
    }
    if(!results.contains("uris") || results["uris"].toStringList().isEmpty()) {
        qWarning() << "No uris returned from file save dialog";
        co_return false;
    }

    QString savedUrlStr = results["uris"].toStringList().first();
    QString savedFile = QUrl{savedUrlStr}.toLocalFile();
    if(savedFile.isEmpty()) {
        qWarning() << "Failed to get saved file path from" << savedUrlStr;
        co_return false;
    }
    if(QFile::exists(savedFile)) {
        if(!QFile::remove(savedFile)) {
            qWarning() << "Failed to remove existing file" << savedFile;
            co_return false;
        }
    }
    if(!QFile::rename(file.toLocalFile(), savedFile)) {
        qWarning() << "Failed to move" << file.toLocalFile() << "to" << savedFile;
        co_return false;
    }

    if(!show) {
        co_return true;
    }

    int fd = open(savedFile.toLocal8Bit().data(), O_RDONLY);
    if(fd == -1) {
        qWarning() << "Failed to open saved file descriptor for" << savedFile;
        co_return false;
    }
    QDBusInterface openURI("org.freedesktop.portal.Desktop",
                           "/org/freedesktop/portal/desktop",
                           "org.freedesktop.portal.OpenURI",
                           QDBusConnection::sessionBus());
    openURI.asyncCall("OpenDirectory", "", QVariant::fromValue(QDBusUnixFileDescriptor{fd}), QVariantMap{});
    close(fd);

    co_return true;
}
