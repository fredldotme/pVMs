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
import Lomiri.Components 1.3
import Lomiri.Components.Popups 1.3
import Lomiri.Components.Themes 1.3
import QtQuick.Layouts 1.3
import QtQuick.Window 2.12
import QMLTermWidget 1.0
import PocketVMs 1.0

MainView {
    id: root
    objectName: 'mainView'
    applicationName: 'pvms.me.fredl'
    automaticOrientation: true
    anchorToKeyboard: true

    readonly property int typicalMargin : units.gu(2)
    property var runningMachineRefs : []
    property bool fullscreenMode : false
    property Page selectedMachinePage : null
    readonly property Machine selectedMachine : selectedMachinePage ? selectedMachinePage.machine : null
    property string errorString : ""

    readonly property Component legacyFilePicker :
        Qt.createComponent("qrc:/LegacyFilePicker.qml",
                           Component.PreferSynchronous);
    readonly property Component lomiriFilePicker :
        Qt.createComponent("qrc:/LomiriFilePicker.qml",
                           Component.PreferSynchronous);
    readonly property Component lomiriFileReceiver :
        Qt.createComponent("qrc:/LomiriFileReceiver.qml",
                           Component.PreferSynchronous);

    property var fileReceiver : null

    function isRegisteredMachine(storage) {
        for (var i = 0; i < runningMachineRefs.length; i++) {
            if (runningMachineRefs[i].key === storage) {
                return true;
            }
        }
        return false;
    }
    function getRegisteredMachine(storage) {
        for (var i = 0; i < runningMachineRefs.length; i++) {
            if (runningMachineRefs[i].key === storage) {
                return runningMachineRefs[i].value;
            }
        }
        return null;
    }
    function registerMachine(machine) {
        for (var i = 0; i < runningMachineRefs.length; i++) {
            // Already registered? Nothing to do!
            if (runningMachineRefs[i].key === machine.storage) {
                return
            }
        }
        runningMachineRefs.push({key: machine.storage, value: machine})
    }
    function unregisterMachine(machine) {
        for (var i = 0; i < runningMachineRefs.length; i++) {
            if (runningMachineRefs[i].key === machine.storage) {
                runningMachineRefs.splice(i, 1);
            }
        }
    }
    function reconnect(machine, vncClient) {
        const socket = machine.storage + "/vnc.sock";
        vncClient.connectToServer(socket, "");
    }

    width: units.gu(45)
    height: units.gu(75)

    Component.onCompleted: {
        VMManager.refreshVMs();
        if (!legacy)
            fileReceiver = lomiriFileReceiver.createObject(root)
    }

    AdaptivePageLayout {
        id: rootLayout

        anchors.fill: parent
        primaryPage: mainPage
        layouts: [
            PageColumnsLayout {
                when: root.width > root.height && !fullscreenMode
                PageColumn {
                    preferredWidth: units.gu(40)
                }
                PageColumn {
                    fillWidth: true
                }
            },
            PageColumnsLayout {
                when: true
                PageColumn {
                    fillWidth: true
                }
            }
        ]

        Page {
            id: mainPage
            header: PageHeader {
                id: header
                title: i18n.tr("Virtual machines")
                trailingActionBar {
                    actions: [
                        Action {
                            iconName: "info"
                            text: i18n.tr("Info")
                            onTriggered: {
                                mainPage.pageStack.addPageToNextColumn(mainPage, about)
                            }
                        },
                        Action {
                            iconName: "add"
                            text: i18n.tr("Add VM")
                            onTriggered: {
                                mainPage.pageStack.addPageToNextColumn(mainPage,
                                                                       addVmComponent.createObject(mainPage))
                            }
                        }
                    ]
                    numberOfSlots: 2
                }
            }
            Label {
                anchors.centerIn: parent
                text: i18n.tr("Please add a VM to continue")
                textSize: Label.Large
                visible: vmListView.model.length <= 0
            }
            LomiriListView {
                id: vmListView
                clip: true
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
                    property Machine machine : isRegisteredMachine(modelData.storage) ?
                                                   getRegisteredMachine(modelData.storage) :
                                                   VMManager.fromQml(modelData);

                    leadingActions: ListItemActions {
                        actions: [
                            Action {
                                iconName: "delete"
                                enabled: !machine.running
                                onTriggered: {
                                    PopupUtils.open(deleteDialog, null, {machine: machine});
                                }
                            }
                        ]
                    }

                    Rectangle {
                        visible: selectedMachine && machine.storage === selectedMachine.storage
                        color: theme.palette.highlighted.base
                        anchors.fill: parent
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
                        var newPage = vmDetailsComponent.createObject(mainPage,
                                                                      { machine : machine })
                        mainPage.pageStack.addPageToNextColumn(mainPage, newPage)
                        selectedMachinePage = newPage;
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
                property bool serialTerminalEnabled : false

                function focusForOsk() {
                    if (serialTerminalEnabled)
                        serialConnection.forceActiveFocus()
                    else
                        viewer.forceActiveFocus()

                    if (Qt.inputMethod.visible)
                        Qt.inputMethod.hide()
                    else
                        Qt.inputMethod.show()
                }

                Timer {
                    id: delayedReconnect
                    onTriggered: reconnect(vmDetails.machine, vncClient)
                    interval: 500
                    repeat: !vncClient.connected
                }

                Connections {
                    target: machine
                    onStarted: {
                        starting = false
                        registerMachine(machine)
                        if (!machine.externalWindowOnly)
                            delayedReconnect.start()
                    }
                    onStopped: {
                        starting = false
                        serialTerminalEnabled = false
                        unregisterMachine(machine)
                        vncClient.disconnect();
                        fullscreenMode = false
                    }
                    onError: {
                        errorString = err;
                        PopupUtils.open(dialog)
                    }
                }

                Component.onCompleted: {
                    if (machine.running && !machine.externalWindowOnly) {
                        reconnect(machine, vncClient)
                    }
                }

                Component.onDestruction: {
                    root.fullscreenMode = false
                    delayedReconnect.stop()
                }

                header: PageHeader {
                    id: vmDetailsHeader
                    title: i18n.tr("VM: ") + machine.name
                    trailingActionBar {
                        actions: [
                            Action {
                                iconName: "settings"
                                text: i18n.tr("Settings")
                                enabled: !machine.running && !starting
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
                                text: !machine.running ? i18n.tr("Start") : i18n.tr("Stop")
                                enabled: !starting
                                onTriggered: {
                                    if (!machine.running) {
                                        starting = machine.start()
                                    } else {
                                        vncClient.disconnect();
                                        machine.stop()
                                        fullscreenMode = false
                                    }
                                }
                            },
                            Action {
                                iconName: "terminal-app-symbolic"
                                text: i18n.tr("Serial console")
                                visible: machine.running
                                onTriggered: {
                                    serialTerminalEnabled = !serialTerminalEnabled
                                    focusForOsk()
                                }
                            },
                            Action {
                                iconName: root.fullscreenMode ? "view-restore" : "view-fullscreen"
                                text: i18n.tr("Show fullscreen")
                                enabled: machine.running && !machine.externalWindowOnly
                                visible: !machine.externalWindowOnly && root.width > root.height
                                onTriggered: {
                                    root.fullscreenMode = !root.fullscreenMode
                                }
                            },
                            Action {
                                iconName: "input-keyboard-symbolic"
                                text: i18n.tr("Keyboard")
                                enabled: machine.running && !machine.externalWindowOnly
                                visible: !machine.externalWindowOnly && machine.running
                                onTriggered: {
                                    focusForOsk()
                                }
                            }
                        ]
                        numberOfSlots: 5
                    }
                }
                VncClient {
                    id: vncClient
                    onConnectedChanged: {
                        console.log("Connected: " + vncClient.connected)
                    }
                }
                Column {
                    anchors.centerIn: parent
                    visible: !machine.running
                    clip: true
                    anchors {
                        top: vmDetailsHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    anchors.topMargin: typicalMargin
                    Icon {
                        width: units.gu(16)
                        name: "system-suspend"
                        color: theme.palette.normal.base
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Label {
                        text: i18n.tr("VM is not running")
                        textSize: Label.Large
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                Column {
                    anchors.centerIn: parent
                    visible: machine.running && (fullscreenMode || machine.externalWindowOnly)
                    clip: true
                    anchors {
                        top: vmDetailsHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    anchors.topMargin: typicalMargin
                    Icon {
                        width: units.gu(16)
                        name: "torch-on"
                        color: theme.palette.normal.activity
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Label {
                        text: i18n.tr("VM window is running & detached")
                        textSize: Label.Large
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
                ActivityIndicator {
                    id: startingActivity
                    running: starting
                    anchors.centerIn: parent
                }
                Rectangle {
                    anchors.fill: viewer
                    color: "black"
                    visible: viewer.visible
                }
                VncOutput {
                    id: viewer
                    client: vncClient
                    anchors {
                        top: vmDetailsHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    visible: machine.running && !machine.externalWindowOnly
                }
                QMLTermWidget {
                    id: serialConnection
                    anchors {
                        top: vmDetailsHeader.bottom
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    font.family: "Monospace"
                    font.pointSize: 12
                    colorScheme: "cool-retro-term"
                    visible: serialTerminalEnabled
                    session: machine.session

                    Component.onCompleted: serialConnection.setUsesMouse(true)

                    onIsBusySelecting: {
                        if (!busy) {
                            PopupUtils.open(serialContextMenue);
                        }
                    }

                    Component {
                        id: serialContextMenue
                        ActionSelectionPopover {
                            id: serialContextMenu

                            contentWidth: units.gu(30)
                            delegate: ListItem {
                                divider.visible: true
                                enabled: action.enabled

                                Item {
                                    anchors {
                                        left: parent.left
                                        right: parent.right
                                        leftMargin: units.gu(3)
                                        rightMargin: units.gu(2)
                                    }

                                    height: childrenRect.height
                                    anchors.verticalCenter: parent.verticalCenter
                                    Label {
                                        text: action.text
                                        textSize: Label.Small
                                        color: theme.palette.normal.overlayText
                                        opacity: action.enabled ? 1.0 : 0.4
                                    }
                                }
                                onClicked: serialContextMenu.hide()
                                visible: action.visible
                                height: units.gu(4.5)
                            }

                            actions: ActionList {
                                Action {
                                    text: i18n.tr("Copy")
                                    enabled: true
                                    onTriggered: serialConnection.copyClipboard();
                                    property bool divider: true
                                }
                                Action {
                                    text: i18n.tr("Paste")
                                    enabled: true
                                    onTriggered: serialConnection.pasteClipboard();
                                }
                            }
                        }
                    }

                    QMLTermScrollbar {
                        terminal: serialConnection
                        width: units.gu(3)
                        Rectangle {
                            opacity: 0.4
                            anchors.margins: 5
                            radius: width * 0.5
                            anchors.fill: parent
                        }
                    }
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
                    "aarch64",
                    "x86_64"
                ]

                property var supportedArchitecturesReadable : [
                    i18n.tr("aarch64 (%1)".arg(VMManager.canVirtualize(supportedArchitectures[0]) ? "fast" : "slow")),
                    i18n.tr("x86_64 (%1)".arg(VMManager.canVirtualize(supportedArchitectures[1]) ? "fast" : "slow"))
                ]

                property bool creating : false
                property var activeTransfer
                property string isoFileUrl : !editMode ? "" : existingMachine.dvd
                property var filePicker : null

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

                Component.onCompleted: {
                    // Ubuntu Touch
                    if (!legacy) {
                        filePicker = lomiriFilePicker.createObject(root)
                    }
                    // Generic/legacy
                    else {
                        filePicker = legacyFilePicker.createObject(root)
                    }
                    filePicker.accepted.connect(function (){
                        for (var i = 0; i < filePicker.fileUrls.length; i++) {
                            isoFileUrl = filePicker.fileUrls[i]
                        }
                    })
                }

                Machine {
                    id: newMachine
                }

                header: PageHeader {
                    id: addVmHeader
                    title: !editMode ? i18n.tr("Add VM") : i18n.tr("Edit VM")
                    trailingActionBar {
                        actions: [
                            Action {
                                iconName: "ok"
                                text: i18n.tr("Save")
                                enabled: !editMode ? (description.text !== "" && isoFileUrl !== "")
                                                   : (description.text !== "")
                                onTriggered: {
                                    creating = true

                                    if (!editMode) {
                                        newMachine.name = description.text
                                        newMachine.arch = supportedArchitectures[architecture.selectedIndex]
                                        newMachine.cores = coresSlider.value.toFixed(0)
                                        newMachine.mem = memSlider.value.toFixed(0)
                                        newMachine.hddSize = hddSizeSlider.value.toFixed(0)
                                        newMachine.dvd = stripFilePath(isoFileUrl);
                                        newMachine.useVirglrenderer = virglrendererCheckbox.checked;
                                        newMachine.enableFileSharing = fileSharingCheckbox.checked;
                                        newMachine.externalWindowOnly =
                                                externalWindowOnlyCheckbox.enabled &&
                                                externalWindowOnlyCheckbox.checked;

                                        if (VMManager.createVM(newMachine)) {
                                            VMManager.refreshVMs();
                                            addVm.pageStack.removePages(addVm)
                                            selectedMachinePage = null
                                        }
                                    } else {
                                        existingMachine.name = description.text
                                        existingMachine.cores = coresSlider.value.toFixed(0)
                                        existingMachine.mem = memSlider.value.toFixed(0)
                                        existingMachine.dvd = stripFilePath(isoFileUrl);
                                        existingMachine.useVirglrenderer = virglrendererCheckbox.checked;
                                        existingMachine.enableFileSharing = fileSharingCheckbox.checked;
                                        existingMachine.externalWindowOnly =
                                                externalWindowOnlyCheckbox.enabled &&
                                                externalWindowOnlyCheckbox.checked;

                                        if (VMManager.editVM(existingMachine)) {
                                            VMManager.refreshVMs();
                                            addVm.pageStack.removePages(addVm)
                                            selectedMachinePage = null
                                        }
                                    }
                                }
                            }
                        ]
                    }
                }

                function openIsoPicker() {
                    filePicker.open()
                }

                ActivityIndicator {
                    id: creatingActivity
                    running: creating
                    anchors.centerIn: parent
                }

                Flickable {
                    id: addVmFlickable
                    width: parent.width
                    height: parent.height - addVmHeader.height
                    y: addVmHeader.height
                    contentHeight: addVmMainColumn.height
                    contentWidth: addVmFlickable.width

                    Column {
                        id: addVmMainColumn
                        x: typicalMargin
                        y: typicalMargin
                        width: parent.width - (typicalMargin*2)
                        height: childrenRect.height + (typicalMargin*2)
                        spacing: typicalMargin / 2

                        TextField {
                            id: description
                            placeholderText: i18n.tr("Description")
                            width: parent.width
                            text: !editMode ? "" : existingMachine.name
                        }

                        OptionSelector {
                            id: architecture
                            text: i18n.tr("Architecture")
                            model: supportedArchitecturesReadable
                            enabled: !editMode
                            selectedIndex: !editMode ? 0 : supportedArchitectures.indexOf(existingMachine.arch)
                        }

                        Column {
                            width: parent.width
                            Label {
                                text: i18n.tr("CPU cores: ") + coresSlider.value.toFixed(0)
                            }

                            Slider {
                                id: coresSlider
                                minimumValue: 1
                                maximumValue: VMManager.maxCores
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
                                text: i18n.tr("RAM: ") + memSlider.value.toFixed(0) + i18n.tr("MB")
                            }

                            Slider {
                                id: memSlider
                                minimumValue: 256
                                maximumValue: VMManager.maxRam
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
                                text: i18n.tr("HDD size: ") + hddSizeSlider.value.toFixed(0) + i18n.tr("GB")
                            }

                            Slider {
                                id: hddSizeSlider
                                minimumValue: 1
                                maximumValue: VMManager.maxHddSize
                                value: 8
                                live: true
                                width: parent.width
                            }
                        }

                        Column {
                            width: parent.width
                            spacing: typicalMargin
                            Label {
                                text: i18n.tr("DVD drive")
                            }

                            Row {
                                width: parent.width
                                Button {
                                    id: isoImport
                                    text: isoFileUrl === "" ? i18n.tr("Pick an ISO") : getFileName(isoFileUrl)
                                    onClicked: openIsoPicker()
                                    width: parent.width - clearIsoButton.width
                                }
                                Button {
                                    id: clearIsoButton
                                    iconName: "media-eject"
                                    width: units.gu(4)
                                    onClicked: {
                                        isoFileUrl = ""
                                    }
                                }
                            }
                        }

                        Row {
                            width: parent.width
                            Switch {
                                id: externalWindowOnlyCheckbox
                                checked: editMode ? existingMachine.externalWindowOnly : false
                                anchors.verticalCenter: externalWindowOnlyHint.verticalCenter
                            }
                            ListItemLayout {
                                id: externalWindowOnlyHint
                                title.text: i18n.tr("Windowed Mode")
                                summary.text: i18n.tr("Improves keyboard, mouse & OpenGL")
                            }
                        }

                        Row {
                            width: parent.width
                            Switch {
                                id: virglrendererCheckbox
                                checked: editMode ? existingMachine.useVirglrenderer : false
                                anchors.verticalCenter: virglHint.verticalCenter
                            }
                            ListItemLayout {
                                id: virglHint
                                title.text: i18n.tr("3D graphics support")
                                summary.text: i18n.tr("Enables 3D in both windowed and headless scenarios")
                            }
                        }

                        Row {
                            width: parent.width
                            Switch {
                                id: fileSharingCheckbox
                                checked: editMode ? existingMachine.enableFileSharing : true
                                anchors.verticalCenter: fileSharingHint.verticalCenter
                            }
                            ListItemLayout {
                                id: fileSharingHint
                                title.text: i18n.tr("Enable file sharing")
                                summary.text: i18n.tr("Accessible via the virtiofs mount tag 'pocketvms'")
                            }
                        }

                        Column {
                            width: parent.width
                            spacing: typicalMargin
                            visible: editMode
                            Label {
                                text: i18n.tr("Reset EFI")
                            }
                            Row {
                                width: parent.width
                                Button {
                                    text: i18n.tr("Reset firmware")
                                    width: parent.width / 2
                                    onClicked: VMManager.resetEFIFirmware(existingMachine)
                                }
                                Button {
                                    text: i18n.tr("Reset NVRAM")
                                    width: parent.width / 2
                                    onClicked: VMManager.resetEFINVRAM(existingMachine)
                                }
                            }
                        }
                    }
                }

                Scrollbar {
                    id: addVmScrollbar
                    flickableItem: addVmFlickable
                    align: Qt.AlignTrailing
                }
            }
        }

        Page {
            id: about
            header: PageHeader {
                id: aboutHeader
                title: i18n.tr("About Pocket VMs")
            }

            Flickable {
                id: aboutFlickable
                width: parent.width
                height: parent.height - aboutHeader.height
                y: aboutHeader.height
                contentHeight: aboutMainColumn.height

                Column {
                    id: aboutMainColumn
                    x: typicalMargin
                    y: typicalMargin
                    width: parent.width - (typicalMargin*2)
                    height: childrenRect.height + (typicalMargin*2)
                    spacing: typicalMargin

                    LomiriShape {
                        width: Math.min(parent.width, parent.height) / 2
                        height: width
                        anchors.horizontalCenter: parent.horizontalCenter
                        source: Image {
                            source: "qrc:/assets/logo.svg"
                        }
                    }

                    Label {
                        width: parent.width
                        text: i18n.tr("Pocket VMs")
                        textSize: Label.Large
                        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                        horizontalAlignment: Text.AlignHCenter
                    }

                    Label {
                        width: parent.width
                        text: i18n.tr("Virtual Machines for your Ubuntu Touch device")
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
                        text: i18n.tr("Donation")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                    }

                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: typicalMargin

                        Label {
                            text: i18n.tr("Via PayPal")
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
                            text: i18n.tr("Via Flattr")
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
                        text: i18n.tr("Copyright notices")
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 16
                    }

                    Label {
                        text: i18n.tr("Licensed under GPL v3")
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
                        text: i18n.tr("Source code on GitHub")
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
                        text: i18n.tr("QEMU (GPL v2 & GPL v3)")
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
                        text: i18n.tr("CuteVNC QML controls by Alberto Mardegan (GPL v3)")
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
                        text: i18n.tr("Icon by Mateo Salta")
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
            Scrollbar {
                flickableItem: aboutFlickable
                align: Qt.AlignTrailing
            }
        }
    }

    Component {
        id: dialog

        Dialog {
            id: dialogue
            title: i18n.tr("QEMU quit unexpectedly!")
            text: i18n.tr("Error: ") + errorString
            Button {
                text: i18n.tr("Ok")
                color: theme.palette.normal.positive
                onClicked: {
                    PopupUtils.close(dialogue);
                }
            }
        }
    }

    Component {
        id: deleteDialog

        Dialog {
            id: deleteDialogue
            title: i18n.tr("Delete machine")
            text: i18n.tr("Would you like to delete this VM?")

            property Machine machine : null

            Button {
                text: i18n.tr("Yes")
                color: theme.palette.normal.positive
                onClicked: {
                    if (selectedMachine && machine.storage === selectedMachine.storage)
                        mainPage.pageStack.removePages(selectedMachinePage)
                    VMManager.deleteVM(machine)
                    VMManager.refreshVMs()
                    PopupUtils.close(deleteDialogue);
                }
            }
            Button {
                text: i18n.tr("No")
                color: theme.palette.normal.negative
                onClicked: {
                    PopupUtils.close(deleteDialogue);
                }
            }
        }
    }
}
