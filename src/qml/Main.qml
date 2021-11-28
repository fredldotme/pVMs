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
import Ubuntu.Components.Themes 1.3
import Ubuntu.Content 1.1
import QtQuick.Layouts 1.3
import PocketVMs 1.0

MainView {
    id: root
    objectName: 'mainView'
    applicationName: 'pvms.me.fredl'
    automaticOrientation: true

    // TODO: Make this pretty with a dark-purplish background and white text
    /*theme.palette: Palette {
        normal.background: UbuntuColors.purple
    }*/

    readonly property int typicalMargin : units.gu(3)

    width: units.gu(45)
    height: units.gu(75)

    Component.onCompleted: {
        VMManager.refreshVMs();
    }

    AdaptivePageLayout {
        id: rootLayout
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
                                mainPage.pageStack.addPageToNextColumn(mainPage,
                                                                       addVmComponent.createObject(mainPage))
                            }
                        },
                        Action {
                            iconName: "info"
                            text: "Info"
                            onTriggered: {
                                mainPage.pageStack.addPageToNextColumn(mainPage, about)
                            }
                        }
                    ]
                    numberOfSlots: 2
                }
            }
            Label {
                anchors.centerIn: parent
                text: "Please add a VM to continue"
                textSize: Label.Large
                visible: vmListView.model.length <= 0
            }
            UbuntuListView {
                id: vmListView
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
                delegate: ListItem {
                    property Machine machine : VMManager.fromQml(modelData);

                    leadingActions: ListItemActions {
                        actions: [
                            Action {
                                iconName: "delete"
                                enabled: !machine.running
                                onTriggered: {
                                    VMManager.deleteVM(machine)
                                    VMManager.refreshVMs()
                                }
                            }
                        ]
                    }

                    ListItemLayout {
                        title.text: machine.name
                        summary.text: machine.arch + ", " + machine.cores + " cores, " + machine.mem + "MB RAM"
                        Icon {
                            id: icon
                            width: units.gu(2)
                            name: !machine.running ? "" : "media-playback-start"
                            color: !machine.running ? theme.palette.normal.base : theme.palette.normal.activity
                        }
                    }
                    onClicked: {
                        mainPage.pageStack.addPageToNextColumn(mainPage,
                                                               vmDetailsComponent.createObject(mainPage,
                                                                                               { machine : machine }))
                    }
                }
            }
        }
        Component {
            id: vmDetailsComponent
            Page {
                id: vmDetails
                property Machine machine : null
                property bool starting : false

                function reconnect() {
                    var port = 5900 + machine.number
                    vncClient.connectToServer("127.0.0.1:" + port, "");
                }

                Connections {
                    target: machine
                    onStarted: {
                        starting = false
                        reconnect()
                    }
                    onStopped: {
                        starting = false
                        vncClient.disconnect();
                    }
                }

                Component.onCompleted: {
                    if (machine.running) {
                        reconnect()
                    }
                }

                header: PageHeader {
                    title: "VM: " + machine.name
                    trailingActionBar {
                        actions: [
                            Action {
                                iconName: "settings"
                                text: "Settings"
                                enabled: !machine.running
                                onTriggered: {
                                    mainPage.pageStack.addPageToNextColumn(mainPage,
                                                                           addVmComponent.createObject(mainPage,
                                                                                                       {
                                                                                                           existingMachine : machine
                                                                                                       }))
                                }
                            },
                            Action {
                                iconName: !machine.running ? "media-playback-start" : "media-playback-stop"
                                text: !machine.running ? "Start" : "Stop"
                                enabled: !starting
                                onTriggered: {
                                    if (!machine.running) {
                                        starting = true;
                                        machine.start()
                                    } else {
                                        machine.stop()
                                    }
                                }
                            },
                            Action {
                                iconName: "input-keyboard-symbolic"
                                text: "Keyboard"
                                enabled: machine.running
                                onTriggered: {
                                    viewer.forceActiveFocus()
                                    Qt.inputMethod.show()
                                }
                            }
                        ]
                        numberOfSlots: 3
                    }
                }
                VncClient {
                    id: vncClient
                    onConnectedChanged: {
                        console.log("Connected to instance")
                    }
                }
                Label {
                    anchors.centerIn: parent
                    text: "VM is not running"
                    textSize: Label.Large
                    visible: !machine.running
                }
                ActivityIndicator {
                    id: startingActivity
                    running: starting
                    anchors.centerIn: parent
                }
                VncOutput {
                    id: viewer
                    client: vncClient
                    anchors.fill: parent
                    visible: machine.running
                }
            }
        }

        Component {
            id: addVmComponent
            Page {
                id: addVm

                property Machine existingMachine : null
                readonly property bool editMode : existingMachine !== null

                property var supportedArchitectures : [
                    i18n.tr("aarch64"),
                    i18n.tr("x86_64")
                ]

                property bool creating : false
                property list<ContentItem> importItems
                property var activeTransfer
                property string isoFileUrl : !editMode ? "" : existingMachine.dvd

                function getFileName(path) {
                    var crumbs = path.split("/").filter(function (element) {
                        return element !== null && element !== "";
                    });

                    if (crumbs.length < 1)
                        return "/"

                    return crumbs[(crumbs.length - 1) % crumbs.length]
                }

                function stripFilePath(path) {
                    if (path.indexOf("file://") !== 0)
                        return path
                    return path.substring(7)
                }

                Machine {
                    id: newMachine
                }

                header: PageHeader {
                    id: addVmHeader
                    title: !editMode ? "Add VM" : "Edit VM"
                    trailingActionBar {
                        actions: [
                            Action {
                                iconName: "ok"
                                text: "Save"
                                enabled: !editMode ? (description.text !== "" && isoFileUrl !== "")
                                                   : true
                                onTriggered: {
                                    creating = true

                                    if (!editMode) {
                                        newMachine.name = description.text
                                        newMachine.arch = supportedArchitectures[architecture.selectedIndex]
                                        newMachine.cores = coresSlider.value
                                        newMachine.mem = memSlider.value
                                        newMachine.hddSize = hddSizeSlider.value
                                        newMachine.dvd = stripFilePath(isoFileUrl);

                                        if (VMManager.createVM(newMachine)) {
                                            VMManager.refreshVMs();
                                            addVm.pageStack.removePages(addVm)
                                        }
                                    } else {
                                        existingMachine.cores = coresSlider.value
                                        existingMachine.mem = memSlider.value
                                        existingMachine.dvd = stripFilePath(isoFileUrl);

                                        if (VMManager.editVM(existingMachine)) {
                                            VMManager.refreshVMs();
                                            addVm.pageStack.removePages(addVm)
                                        }
                                    }
                                }
                            }
                        ]
                    }
                }

                onImportItemsChanged: {
                    if (importItems.length < 1 || importItems > 1)
                        return;
                    isoFileUrl = importItems[0].url
                }

                function openIsoPicker() {
                    var peer = null
                    for (var i = 0; i < contentPeerModel.peers.length; ++i) {
                        var p = contentPeerModel.peers[i]
                        if (p.appId.indexOf("com.ubuntu.filemanager_") === 0) {
                            peer = p
                        }
                    }

                    peer.selectionType = ContentTransfer.Single
                    activeTransfer = peer.request()
                }

                ContentPeerModel {
                    id: contentPeerModel
                    contentType: ContentType.Documents
                    handler: ContentHandler.Source
                }
                ContentTransferHint {
                    id: importHint
                    anchors.fill: parent
                    activeTransfer: activeTransfer
                }

                Connections {
                    target: activeTransfer
                    onStateChanged: {
                        if (activeTransfer.state === ContentTransfer.Charged) {
                            importItems = activeTransfer.items;
                        }
                    }
                }

                Flickable {
                    anchors {
                        top: addVmHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }

                    // Hack around OptionSelector imploding when pressed
                    // Allows scrolling past the edge but better than nothing...
                    contentHeight: addVmMainColumn.height + architecture.height

                    ActivityIndicator {
                        id: creatingActivity
                        running: creating
                        anchors.centerIn: parent
                    }

                    Column {
                        id: addVmMainColumn
                        anchors.fill: parent
                        spacing: typicalMargin

                        TextField {
                            id: description
                            placeholderText: "Description"
                            width: parent.width
                            enabled: !editMode
                            text: !editMode ? "" : existingMachine.name
                        }

                        OptionSelector {
                            id: architecture
                            text: i18n.tr("Architecture")
                            model: supportedArchitectures
                            enabled: !editMode
                            selectedIndex: !editMode ? 0 : supportedArchitectures.indexOf(existingMachine.arch)
                        }

                        Column {
                            width: parent.width
                            Label {
                                text: "CPU cores: " + coresSlider.value.toFixed(0)
                            }

                            Slider {
                                id: coresSlider
                                minimumValue: 1
                                maximumValue: 4
                                stepSize: 1
                                value: !editMode ? 1 : existingMachine.cores
                                live: true
                                width: parent.width
                                function formatValue(v) {
                                    return v.toFixed(0)
                                }
                            }
                        }

                        Column {
                            width: parent.width
                            Label {
                                text: "RAM: " + memSlider.value.toFixed(0) + "MB"
                            }

                            Slider {
                                id: memSlider
                                minimumValue: 256
                                maximumValue: 4096
                                stepSize: 256
                                value: !editMode ? 1024 : existingMachine.mem
                                live: true
                                width: parent.width
                                function formatValue(v) {
                                    return v.toFixed(0)
                                }
                            }
                        }

                        Column {
                            width: parent.width
                            visible: !editMode
                            Label {
                                text: "HDD size: " + hddSizeSlider.value.toFixed(0) + "GB"
                            }

                            Slider {
                                id: hddSizeSlider
                                minimumValue: 1
                                maximumValue: 20
                                value: 8
                                live: true
                                width: parent.width
                            }
                        }

                        Row {
                            width: parent.width
                            Button {
                                id: isoImport
                                text: isoFileUrl === "" ? "Pick an ISO" : getFileName(isoFileUrl)
                                onClicked: openIsoPicker()
                                width: parent.width - clearIsoButton.width
                            }
                            Button {
                                id: clearIsoButton
                                iconName: "edit-clear"
                                width: units.gu(4)
                                onClicked: {
                                    isoFileUrl = ""
                                }
                            }
                        }
                    }
                }
            }
        }
        Page {
            id: about
            header: PageHeader {
                id: aboutHeader
                title: "About Pocket VMs"
            }
            Flickable {
                anchors {
                    top: aboutHeader.bottom
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                contentHeight: aboutMainColumn.height

                Column {
                    id: aboutMainColumn
                    width: parent.width
                    spacing: typicalMargin

                    Label {
                        width: parent.width
                        text: qsTr("Pocket VMs")
                        textSize: Label.Large
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        width: parent.width
                        text: qsTr("Virtual Machines for your Ubuntu Touch device")
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: typicalMargin
                            rightMargin: typicalMargin
                        }
                    }

                    Label {
                        text: qsTr("Donation")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                    }

                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: typicalMargin

                        Label {
                            text: qsTr("Via PayPal")
                            font.underline: true
                            textSize: Label.Large
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    Qt.openUrlExternally("https://paypal.me/beidl")
                                }
                            }
                        }

                        /*Label {
                            text: qsTr("Via Flattr")
                            font.underline: true
                            textSize: Label.Large
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    Qt.openUrlExternally("https://flattr.com/@beidl")
                                }
                            }
                        }*/
                    }

                    Label {
                        text: qsTr("Copyright notices")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                    }

                    Label {
                        text: qsTr("Licensed under GPL v3")
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: typicalMargin
                            rightMargin: typicalMargin
                        }
                    }

                    Label {
                        text: qsTr("Source code on GitHub")
                        font.underline: true
                        textSize: Label.Small
                        horizontalAlignment: Text.AlignHCenter
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                Qt.openUrlExternally("https://github.com/fredldotme/pVMs")
                            }
                        }
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: typicalMargin
                            rightMargin: typicalMargin
                        }
                    }

                    Label {
                        text: qsTr("QEMU (GPL v2 & GPL v3)")
                        textSize: Label.XSmall
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: typicalMargin
                            rightMargin: typicalMargin
                        }
                    }
                    Label {
                        text: qsTr("CuteVNC QML controls by Alberto Mardegan (GPL v3)")
                        textSize: Label.XSmall
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                        anchors {
                            left: parent.left
                            right: parent.right
                            leftMargin: typicalMargin
                            rightMargin: typicalMargin
                        }
                    }
                }
            }
        }
    }
}
