import QtQuick 2.4
import QtQml 2.2
import Lomiri.Components 1.3
import Lomiri.Components.Popups 1.3

//import "../actions" as FMActions

QtObject {
    id: rootItem

    property var folderListPage
    property var folderModel
    property var fileOperationDialog
    property var openDefault
    property var openFile

    function listLongPress(model) {
        fileSelectorMode = true
        fileSelector.fileSelectorComponent = pageStack
        folderModel.model.selectionObject.select(model.index,false,true)
    }

    function itemClicked(model, loading) {
        if (model.isBrowsable) {    // Item is dir
            console.log("browsable path=", model.filePath,
                        "isRemote=", model.isRemote,
                        "needsAuthentication=", model.needsAuthentication)

            // if (!isContentHub && fileSelectorMode) {
            //     folderModel.model.selectionObject.select(model.index,false,true)
            // }

            /*else*/ {
                var isReadableDir = model.isReadable && model.isExecutable

                if (isReadableDir) {
                    console.log("Changing to dir", model.filePath)
                    loading.running = true // no point in setting this to false later, as the view will change and the loading indicator will be destroyed
                    folderModel.goTo(model.filePath)
                } else {
                    var props = {
                        title: i18n.tr("Folder not accessible"),
                        // TRANSLATORS: this refers to a folder name
                        text: i18n.tr("Can not access %1").arg(model.fileName)
                    }

                    PopupUtils.open(Qt.resolvedUrl("../dialogs/NotifyDialog.qml"), mainView, props)
                }
            }
        }

        else {  // Item is file
            console.log("Non dir clicked")
            var path = model.filePathFull
            //var archiveType = folderModel.getArchiveType(model.fileName)
            console.log(path)
            console.log(model.mimeType)

            openFile(path)

            // if (fileSelectorMode) {
            //     folderModel.model.selectionObject.select(model.index,false,true)
            // } else if (!folderSelectorMode){
            //     if (openDefault) {
            //         if (model.mimeType.indexOf("image/") != -1) {
            //             Qt.openUrlExternally("photo://" + path)
            //         } else if (model.mimeType.indexOf("audio/") != -1) {
            //             Qt.openUrlExternally("music://" + path)
            //         } else if (model.mimeType.indexOf("video/") != -1) {
            //             Qt.openUrlExternally("video://" + path)
            //         } else if (model.mimeType.indexOf("pdf") != -1 || model.mimeType.indexOf("vnd") != -1) {
            //             Qt.openUrlExternally("document://" + path)
            //         } else if (archiveType !== "") {
            //             folderListPage.openFile(model, true)
            //         } else {
            //             openAdvanced(model)
            //         }
            //     } else {
            //         openAdvanced(model)
            //     }
            // }
        }
    }

    function itemDateAndSize(model) {
        var strDate = Qt.formatDateTime(model.modifiedDate, Qt.DefaultLocaleShortDate);
        //local file systems always have date and size for both files and directories
        //remote file systems may have not size for directories, it comes as "Unknown"
        if (strDate && model.fileSize) {
            strDate += ", " + model.fileSize //show the size even it is "Unknown"
        }
        return strDate;
    }

    function openAdvanced(model) {
        if (model.isLocal) {    // Item is local file
            var archiveType = folderModel.getArchiveType(model.fileName)

            var props = {
                model: model,
                previewButtonVisible: model.mimeType.indexOf("image/") + model.mimeType.indexOf("audio/") + model.mimeType.indexOf("video/") > -3,
                extractButtonVisible: archiveType !== ""
            }

            var popup = PopupUtils.open(Qt.resolvedUrl("../dialogs/OpenWithDialog.qml"), mainView, props)

            popup.showPreview.connect(function() {
                if(model.mimeType.indexOf("image/") !== -1) {
                    let props = {
                        filesModel: folderModel.model,
                        initialMedia: model
                    }
                    pageStack.push(Qt.resolvedUrl("../ui/MediaViewer.qml"), props)
                } else {
                    Qt.openUrlExternally("video://" + filePath)
                }
            })

            popup.extractArchive.connect(function() {
                folderModel.extractArchive(model.filePath, model.fileName, archiveType)
            })

            popup.openWith.connect(function() {
                openLocalFile(model.filePath)
            })

            popup.showProperties.connect(function() {
                PopupUtils.open(Qt.resolvedUrl("../ui/FileDetailsPopover.qml"), mainView,{ "model": model })
            })

        } else {    // Item is remote file
            //download and open later when the signal downloadTemporaryComplete() arrives
            folderModel.model.downloadAsTemporaryFile(model.index)
        }
    }

    property ActionList leadingActions: ActionList {
        // FMActions.Delete {
        //     visible: folderModel.model.isWritable && importMode
        //     onTriggered: {
        //         var props = {
        //             "folderModel": folderModel.model,
        //             "fileOperationDialog": fileOperationDialog,
        //             "filePath" : model.filePath,
        //             "fileName" : model.fileName
        //         }
        //         PopupUtils.open(Qt.resolvedUrl("../dialogs/ConfirmSingleDeleteDialog.qml"), mainView, props)
        //     }
        // }
    }

    property ActionList trailingActions: ActionList {
        // FMActions.OpenAdvanced {
        //     visible: openDefault && !model.isBrowsable
        //     onTriggered: openAdvanced(model)
        // }

        // FMActions.ArchiveExtract {
        //     visible: typeof (model) !== "undefined" && folderModel.getArchiveType(model.fileName) !== "" && importMode
        //     onTriggered: folderListPage.openFile(model, true)
        // }

        // FMActions.Properties {
        //     onTriggered: {
        //         var props = {
        //             "model": model
        //         }
        //         PopupUtils.open(Qt.resolvedUrl("../ui/FileDetailsPopover.qml"), mainView, props)
        //     }
        // }

        // FMActions.Rename {
        //     visible: folderModel.model.isWritable && importMode
        //     onTriggered: {
        //         var props = {
        //             "modelRow" : model.index,
        //             "inputText" : model.fileName,
        //             "placeholderText" : model.fileName,
        //             "folderModel": folderModel.model
        //         }

        //         var popup = PopupUtils.open(Qt.resolvedUrl("../dialogs/ConfirmRenameDialog.qml"), mainView, props)

        //         popup.accepted.connect(function(inputText) {
        //             console.log("Rename accepted", inputText)
        //             if (inputText !== '') {
        //                 console.log("Rename commensed, modelRow/inputText", model.index, inputText.trim())
        //                 if (folderModel.model.rename(model.index, inputText.trim()) === false) {
        //                     var props = {
        //                         title: i18n.tr("Could not rename"),
        //                         text: i18n.tr("Insufficient permissions, name contains special chars (e.g. '/'), or already exists")
        //                     }
        //                     PopupUtils.open(Qt.resolvedUrl("../dialogs/NotifyDialog.qml"), mainView, props)
        //                 }
        //             } else {
        //                 console.log("Empty new name given, ignored")
        //             }
        //         })
        //     }
        // }

        // FMActions.FileCut {
        //     visible: folderModel.model.isWritable && importMode
        //     onTriggered: {
        //         folderModel.model.cutIndex(model.index)
        //     }
        // }

        // FMActions.FileCopy {
        //     visible: importMode
        //     onTriggered: {
        //         folderModel.model.copyIndex(model.index)
        //     }
        // }

        // FMActions.Terminal {
        //     visible: typeof (model) !== "undefined" && model.isDir
        //     onTriggered: Qt.openUrlExternally("terminal://?path=" + model.filePath)
        // }

        // FMActions.Share {
        //     visible: typeof (model) !== "undefined" && !model.isDir && importMode
        //     onTriggered: folderListPage.openFile(model, true)
        // }
    }
}
