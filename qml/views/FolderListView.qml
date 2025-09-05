/*
 * Copyright (C) 2013 Canonical Ltd
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
import QtQuick 2.7
import Lomiri.Components 1.3
import Lomiri.Components.Popups 1.3

ScrollView {
    id: folderListView

    property var folderListPage
    property var fileOperationDialog
    property var folderModel
    property var openDefault

    property alias footer: root.footer
    property alias header: root.header

    ListView {
        id: root
        anchors.fill: parent
        model: folderModel
        boundsBehavior: Flickable.DragOverBounds

        PullToRefresh {
            onRefresh: {
                refreshing = true
                folderModel.goTo(folderModel.filePath)
                refreshing = false
            }
        }

        delegate: FolderListDelegate {
            id: delegate

            title: model.stylizedFileName
            //subtitle: __delegateActions.itemDateAndSize(model)
            summary: folderModel.getSearchRecursive() && folderModel.getSearchString() ?
                         model.filePath.toString().replace(folderModel.homePath(), "~") : ""
            iconName: model.iconName
            showProgressionSlot: model.isBrowsable
            isSelected: model.isSelected
            path: model.filePath

            // property var __delegateActions: FolderDelegateActions {
            //     folderListPage: folderListView.folderListPage
            //     folderModel: folderListView.folderModel
            //     fileOperationDialog: folderListView.fileOperationDialog
            //     openDefault: folderListView.openDefault
            // }

            // leadingActions: ListItemActions {
            //     // Children is an alias for 'actions' property, this way we don't get any warning about non-NOTIFYable props
            //     actions: __delegateActions.leadingActions.children
            // }

            // trailingActions: ListItemActions {
            //     // Children is an alias for 'actions' property, this way we don't get any warning about non-NOTIFYable props
            //     actions: __delegateActions.trailingActions.children
            // }

            // onClicked: __delegateActions.itemClicked(model)
            // onPressAndHold: {
            //     folderModel.primSelItem = model
            //     __delegateActions.listLongPress(model)
            // }
        }

        // section.property: "isDir"
        // section.delegate: SectionDivider {
        //     text: section == "true" ? i18n.tr("Directories") : i18n.tr("Files")
        // }
    }
}
