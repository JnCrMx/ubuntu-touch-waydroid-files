/*
 * Copyright (C) 2014 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Arto Jalkanen <ajalkane@gmail.com>
 */
import QtQuick 2.4
import Lomiri.Components 1.3
import Lomiri.Content 1.3

import ADB 1.0

Page {
    id: root

    header: PageHeader {
        id: header
        title: i18n.tr("Open with")
    }

    property var activeTransfer

    property string devicePath
    property ADBClient adbClient

    property bool share: false
    property bool cleanup: false

    property bool transferInProgress: false

    Component.onCompleted: {

    }

    Component {
        id: resultComponent
        ContentItem {}
    }

    ContentPeerPicker {
        id: peerPicker
        anchors {
            top: header.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: units.gu(2)
        }

        showTitle: false

        // Type of handler: Source, Destination, or Share
        handler: root.share ? ContentHandler.Share : ContentHandler.Destination
        contentType: ContentType.Pictures

        onPeerSelected: {
            root.activeTransfer = peer.request();
            root.activeTransfer.stateChanged.connect(function() {
                console.log("curTransfer StateChanged: " + root.activeTransfer.state);
                if(root.activeTransfer.state === ContentTransfer.InProgress && !transferInProgress) {
                    transferInProgress = true;
                    adbClient.pullFile(root.devicePath).then(function(url) {
                        if(!url) {
                            console.log("Pull failed, aborting transfer");
                            root.activeTransfer.state = ContentTransfer.Aborted;
                            return;
                        }
                        console.log("Pulled file to " + url);
                        root.activeTransfer.items = [ resultComponent.createObject(root, {"url": url}) ];
                        root.activeTransfer.state = ContentTransfer.Charged;

                        console.log("curTransfer StateChanged by us: " + root.activeTransfer.state);
                    })
                }
                if(root.activeTransfer.state === ContentTransfer.Finalized || root.activeTransfer.state === ContentTransfer.Aborted) {
                    adbClient.cleanupPulledFiles();
                    pageStack.pop();
                }
            })
        }

        onCancelPressed: {
            pageStack.pop();
        }
    }
}
