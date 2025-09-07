/*
 * Copyright (C) 2025 UBports Foundation
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License 3 as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */

import QtQuick 2.12
import Lomiri.Components 1.3
import QtQuick.Layouts 1.12

import "../components" as Components

Item {
    id: rootItem

    readonly property bool noActions: leadingActionBar.actions.length === 0 && trailingActionBar.actions.length === 0

    property alias folderModel: pathHistoryRow.folderModel
    property alias leadingActionBar: leadingActionBar
    property alias trailingActionBar: trailingActionBar

    anchors { left: parent.left; right: parent.right }
    implicitHeight: noActions ? units.gu(4) : units.gu(5)

    RowLayout {
        anchors.fill: parent

        ActionBar {
            id: leadingActionBar

            Layout.fillHeight: true
            Layout.topMargin: units.gu(0.1)
            Layout.bottomMargin: units.gu(0.1)

            visible: actions.length > 0
            numberOfSlots: 5
        }

        Components.PathHistoryRow {
            id: pathHistoryRow

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            // Adds visual separator between history row and action buttons
            Rectangle {
                visible: !rootItem.noActions
                anchors.fill: parent
                color: "transparent"
                border {
                    width: units.dp(2)
                    color: theme.palette.normal.base
                }
            }
        }

        ActionBar {
            id: trailingActionBar

            Layout.fillHeight: true
            Layout.topMargin: units.gu(0.1)
            Layout.bottomMargin: units.gu(0.1)

            visible: actions.length > 0
            numberOfSlots: 5
        }
    }

    // Top and bottom dividers
    Components.HorizontalDivider {
        visible: !rootItem.noActions
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
    }

    Components.HorizontalDivider {
        visible: !rootItem.noActions
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
