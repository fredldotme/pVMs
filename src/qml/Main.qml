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
//import QtQuick.Controls 2.2
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

    AdaptivePageLayout {
        anchors.fill: parent
        primaryPage: page1
        Page {
            id: page1
            header: PageHeader {
                id: header
                title: "Pocket VMs"
            }
            Column {
                anchors.top: header.bottom
                Button {
                    text: "Add Page2 above " + page1.title
                    onClicked: page1.pageStack.addPageToCurrentColumn(page1, page2)
                }
                Button {
                    text: "Add Page3 next to " + page1.title
                    onClicked: page1.pageStack.addPageToNextColumn(page1, page3)
                }
            }
        }
        Page {
            id: page2
            header: PageHeader {
                title: "VM"
            }
        }
        Page {
            id: page3
            header: PageHeader {
                title: "Add VM"
            }
        }
    }
}
