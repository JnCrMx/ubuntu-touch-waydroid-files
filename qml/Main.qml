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

    property var activeTransfer: null
    property string mode: "normal" // normal, import, export

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
        root.activeTransfer = activeTransfer
        root.mode = importMode ? "import" : "export"
        console.log("Starting transfer in mode " + root.mode + ": " + JSON.stringify(activeTransfer))
    }
    function finishTransfer() {
        root.mode = "normal"
        model.selectedFile = ""
        root.activeTransfer = null
        Qt.quit()
    }

    Component {
        id: resultComponent
        ContentItem {}
    }
    function exportFile(path) {
        console.log("Exporting file", path)
        root.activeTransfer.stateChanged.connect(function() {
            console.log("export transfer state changed: " + root.activeTransfer.state);
            if(root.activeTransfer.state === ContentTransfer.Finalized || root.activeTransfer.state === ContentTransfer.Aborted) {
                client.cleanupPulledFiles();

                finishTransfer()
            }
        });
        client.pullFile(path).then(function(url) {
            if(!url) {
                console.log("Pull failed, aborting transfer");
                root.activeTransfer.state = ContentTransfer.Aborted;
                return;
            }
            console.log("Pulled file to " + url);
            root.activeTransfer.items = [ resultComponent.createObject(root, {"url": url}) ];
            root.activeTransfer.state = ContentTransfer.Charged;
        })
    }
    function importFile(path) {
        console.log("Importing file to ", path)
        let hostUrl = root.activeTransfer.items.length > 0 ? root.activeTransfer.items[0].url : null
        if(!hostUrl || !hostUrl.toString().startsWith("file://")) {
            console.log("Invalid URL, aborting transfer");
            root.activeTransfer.state = ContentTransfer.Aborted;
            finishTransfer()
            return;
        }
        let filename = hostUrl.toString().substring(7).split("/").pop()
        let devicePath = path + (path.endsWith("/") ? "" : "/") + filename
        console.log("Importing file " + hostUrl + " to device path " + devicePath)
        // TODO: check if file exists and ask for overwrite
        root.activeTransfer.stateChanged.connect(function() {
            console.log("import transfer state changed: " + root.activeTransfer.state);
            if(root.activeTransfer.state === ContentTransfer.Finalized || root.activeTransfer.state === ContentTransfer.Aborted) {
                finishTransfer()
            }
        });
        client.pushFileFromUrl(hostUrl, devicePath).then(function(success) {
            if(!success) {
                console.log("Push failed, aborting transfer");
                // TODO: show error message
                root.activeTransfer.state = ContentTransfer.Aborted;
                return;
            }
            // TODO: show success message
            console.log("Pushed file to " + devicePath);
            root.activeTransfer.finalize();

            model.goTo(path); // refresh current folder
        });
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
        Action {
            id: actionCancel
            iconName: "close"
            text: i18n.tr("Cancel")
            enabled: root.mode !== "normal"
            onTriggered: {
                root.activeTransfer.state = ContentTransfer.Aborted
                finishTransfer()
            }
        }
        Action {
            id: actionSelect
            iconName: "ok"
            text: i18n.tr("Select")
            enabled: root.mode !== "normal" && (
                root.mode === "import" ||
                root.mode === "export" && model.selectedFile !== ""
            )
            onTriggered: {
                if(mode === "export") {
                    exportFile(model.selectedFile)
                } else if(mode === "import") {
                    importFile(model.currentPath)
                }
            }
        }
    }

    Component {
        id: folderListView

        PageStack {
            id: pageStack

            function openFile(path) {
                console.log("Opening file", path, "in mode", mode)
                if(mode === "normal") {
                    pageStack.push(Qt.resolvedUrl("content-hub/FileOpener.qml"), {
                        adbClient: client,
                        devicePath: path,
                        share: false,
                        cleanup: true
                    })
                } else if(mode === "export") {
                    if(model.selectedFile === path) {
                        model.selectedFile = ""
                    } else {
                        model.selectedFile = path
                    }
                }
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
                    trailingActionBar.actions: root.mode === "normal" ? [] : [ actionCancel, actionSelect ]
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
