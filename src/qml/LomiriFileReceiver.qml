import QtQuick 2.12
import Lomiri.Components 1.3
import Lomiri.Content 1.0
import PocketVMs 1.0

QtObject {
    property var importItems : []
    Connections {
        target: ContentHub

        onImportRequested: {
            if (VMManager.vms.count < 1)
                return;

            importItems = transfer.items;
            PopupUtils.open(contentHubDialog);
        }

        onShareRequested: {
            if (VMManager.vms.count < 1)
                return;

            importItems = transfer.items;
            PopupUtils.open(contentHubDialog);
        }
    }

    Component {
        id: contentHubDialog

        Dialog {
            id: contentHubDialogue
            contentWidth: (root.width / 3) * 2
            contentHeight: (root.height / 3) * 2

            Column {
                width: contentHubDialogue.contentWidth
                height: contentHubDialogue.contentHeight

                ListItemLayout {
                    id: importHeader
                    title.text: i18n.tr("Drop the file in a VM")
                    summary.text: i18n.tr("Select a machine")
                    width: parent.width

                    Button {
                        text: i18n.tr("Cancel")
                        color: theme.palette.normal.negative
                        x: parent.width - width - units.gu(4)

                        onClicked: {
                            PopupUtils.close(contentHubDialogue);
                        }
                    }
                }

                Column {
                    anchors.centerIn: parent
                    visible: contentHubVmListView.model.count === 0
                    anchors.topMargin: typicalMargin

                    ListItemLayout {
                        title.text: i18n.tr("No machines available")
                        summary.text: i18n.tr("Add a machine in the main app view to continue")
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                Repeater {
                    id: contentHubVmListView
                    model: VMManager.vms
                    width: parent.width
                    anchors.bottom: importHeader.bottom
                    delegate: ListItem {
                        property Machine machine : isRegisteredMachine(modelData.storage) ?
                                                       getRegisteredMachine(modelData.storage) :
                                                       VMManager.fromQml(modelData);

                        enabled: machine.enableFileSharing

                        ListItemLayout {
                            title.text: machine.name
                            summary.text: machine.enableFileSharing ? i18n.tr("Available") :
                                                                      i18n.tr("File sharing disabled in settings")
                        }

                        onClicked: {
                            for (var i = 0; i < importItems.length; i++) {
                                machine.importIntoShare(importItems[i].url)
                            }
                            PopupUtils.close(contentHubDialogue);
                        }
                    }
                }
            }
        }
    }
}