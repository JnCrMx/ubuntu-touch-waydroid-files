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

import QtQuick 2.7
import QtQuick.Controls 2.2
import Lomiri.Components 1.3
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import Lomiri.Content 1.3

import ADB 1.0

import "views"
import "ui"

MainView {
    id: root
    objectName: 'mainView'
    applicationName: "waydroid-files.jcm"
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    ADBClient {
        id: client

        onDeviceFound: {
            console.log("Connected to ADB server")

            loader.item.text = i18n.tr("Locating home folder...")
            client.findFirstAccessibleFolder(["/sdcard", "/storage/emulated/0", "/home/phablet", "/"]).then(function(path) {
                console.log("Found accessible folder: " + path)
                loader.item.text = i18n.tr("Opening folder %1...").arg(path)
                model.goTo(path).then(function() {
                    console.log("Loaded initial folder")
                    loader.sourceComponent = folderListView
                })
            })
        }
    }

    ADBFolderModel {
        id: model
        adbClient: client
        basePath: "/"
    }

    function startTransfer(activeTransfer, importMode) {
        console.log(activeTransfer)
        console.log(activeTransfer.state)
        console.log(activeTransfer.items[0].text)
        console.log(activeTransfer.items[0].url)
        console.log(importMode)
    }

    Connections {
        target: ContentHub
        onExportRequested: startTransfer(transfer, false)
        onImportRequested: startTransfer(transfer, true)
        onShareRequested: startTransfer(transfer, true)
    }

    Loader {
        id: loader
        sourceComponent: loadingIndicator
        anchors.fill: parent
    }

    Component {
        id: loadingIndicator
        Page {
            property alias text: loadingText.text

            header: PageHeader {
                id: header
                title: i18n.tr('Waydroid Files')
            }
            ActivityIndicator {
                anchors.centerIn: parent
                running: true
            }
            Text {
                id: loadingText
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.verticalCenter
                anchors.topMargin: units.gu(4)
                text: i18n.tr("Waiting for device...")
            }
        }
    }

    ActionContext {
        Action {
            id: actionGoBack
            text: i18n.tr("Go back")
            iconName: "back"
            onTriggered: model.goBack()
            enabled: model.canGoBack
        }
        Action {
            id: actionGoForward
            text: i18n.tr("Go forward")
            iconName: "go-next"
            onTriggered: model.goForward()
            enabled: model.canGoForward
        }
    }

    Component {
        id: folderListView

        PageStack {
            id: pageStack

            function openFile(path) {
                pageStack.push(Qt.resolvedUrl("content-hub/FileOpener.qml"), {
                    adbClient: client,
                    devicePath: path,
                    share: false,
                    cleanup: true
                })
            }

            Component.onCompleted: {
                pageStack.push(folderListPage)
            }

            Page {
                id: folderListPage
                visible: false

                header: PathHistoryToolbar {
                    id: header
                    folderModel: model

                    leadingActionBar.actions: [ actionGoForward, actionGoBack ]
                }

                FolderListView {
                    anchors {
                        top: header.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }

                    folderModel: model
                    openFile: pageStack.openFile
                }
            }
        }
    }
}
