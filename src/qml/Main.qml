/*
 * Copyright (C) 2021  Alfred Neumayer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * pvms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.7
import Ubuntu.Components 1.3
import Ubuntu.Components.ListItems 1.3 as ListItem
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0
import PocketVMs 1.0

MainView {
    id: root
    objectName: 'mainView'
    applicationName: 'pvms.me.fredl'
    automaticOrientation: true

    width: units.gu(45)
    height: units.gu(75)

    property var selectedMachine : null

    Component.onCompleted: {
        VMManager.refreshVMs();
    }

    AdaptivePageLayout {
        anchors.fill: parent
        primaryPage: mainPage
        Page {
            id: mainPage
            header: PageHeader {
                id: header
                title: "Pocket VMs"
                trailingActionBar {
                    actions: [
                        Action {
                            iconName: "add"
                            text: "Add VM"
                            onTriggered: {
                                mainPage.pageStack.addPageToNextColumn(mainPage, addVm)
                            }
                        },
                        Action {
                            iconName: "info"
                            text: "Info"
                            onTriggered: {
                            }
                        }
                    ]
                    numberOfSlots: 2
                }
            }
            UbuntuListView {
                anchors {
                    top: header.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

                model: VMManager.vms
                pullToRefresh {
                    enabled: true
                    refreshing: VMManager.refreshing
                    onRefresh: VMManager.refreshVMs()
                }
                delegate: ListItem.Expandable {
                    property Machine machine : VMManager.fromQml(modelData);
                    ListItemLayout {
                        title.text: machine.name
                    }
                    onClicked: {
                        selectedMachine = machine
                        mainPage.pageStack.addPageToNextColumn(mainPage, vmDetails)
                    }
                }
            }
        }
        Page {
            id: vmDetails
            header: PageHeader {
                title: "VM"
                trailingActionBar {
                    actions: [
                        Action {
                            iconName: "settings"
                            text: "Settings"
                        },
                        Action {
                            iconName: "media-playback-start"
                            text: "Start"
                        }
                    ]
                    numberOfSlots: 2
                }
            }
        }
        Page {
            id: addVm
            header: PageHeader {
                title: "Add VM"
            }
        }
    }
}
