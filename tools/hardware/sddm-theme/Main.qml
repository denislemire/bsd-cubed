// SPDX-License-Identifier: BSD-2-Clause
import QtQuick 2.15

Rectangle {
    id: root
    width: 1280
    height: 800
    color: "#0a0c0f"

    readonly property color accent: config.accent ? config.accent : "#b33a34"
    readonly property string uiFont: config.font ? config.font : "Noto Sans"
    property int sessionIndex: sessionModel.lastIndex
    property date now: new Date()

    function tryLogin() {
        message.text = ""
        sddm.login(userField.text, passwordField.text, sessionIndex)
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: root.now = new Date()
    }

    Connections {
        target: sddm
        function onLoginFailed() {
            message.text = "That did not work."
            passwordField.text = ""
            passwordField.forceActiveFocus()
        }
        function onLoginSucceeded() { }
        function onInformationMessage(msg) { message.text = msg }
    }

    Text {
        id: clock
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Math.max(48, parent.height * 0.11)
        color: "#e8eaed"
        font.family: root.uiFont
        font.pixelSize: Math.min(72, parent.width / 14)
        font.weight: Font.Light
        text: Qt.formatTime(root.now, "h:mm AP")
        Accessible.name: text
    }

    Text {
        id: dateLabel
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: clock.bottom
        anchors.topMargin: 6
        color: "#8d939c"
        font.family: root.uiFont
        font.pixelSize: 16
        font.letterSpacing: 1.2
        text: Qt.formatDate(root.now, "yyyy-MM-dd")
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: dateLabel.bottom
        anchors.topMargin: 14
        color: "#6e747e"
        font.family: root.uiFont
        font.pixelSize: 11
        font.letterSpacing: 6
        font.capitalization: Font.AllUppercase
        text: "FreeBSD"
    }

    Rectangle {
        id: card
        width: Math.min(380, parent.width - 48)
        height: form.implicitHeight + 48
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: dateLabel.bottom
        anchors.topMargin: 72
        color: "#14181e"
        border.color: "#262c34"
        border.width: 1
        radius: 12

        Column {
            id: form
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 24
            spacing: 12

            Rectangle {
                width: parent.width
                height: 44
                radius: 8
                color: "#0a0c0f"
                border.width: 1
                border.color: userField.activeFocus ? root.accent : "#2a3038"

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    verticalAlignment: Text.AlignVCenter
                    color: "#5c6370"
                    font.family: root.uiFont
                    font.pixelSize: 14
                    text: "Username"
                    visible: userField.text.length === 0
                }
                TextInput {
                    id: userField
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#e8eaed"
                    font.family: root.uiFont
                    font.pixelSize: 14
                    clip: true
                    selectByMouse: true
                    text: userModel.lastUser ? userModel.lastUser : ""
                    KeyNavigation.tab: passwordField
                    Keys.onReturnPressed: root.tryLogin()
                    Keys.onEnterPressed: root.tryLogin()
                }
            }

            Rectangle {
                width: parent.width
                height: 44
                radius: 8
                color: "#0a0c0f"
                border.width: 1
                border.color: passwordField.activeFocus ? root.accent : "#2a3038"

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    verticalAlignment: Text.AlignVCenter
                    color: "#5c6370"
                    font.family: root.uiFont
                    font.pixelSize: 14
                    text: "Password"
                    visible: passwordField.text.length === 0
                }
                TextInput {
                    id: passwordField
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#e8eaed"
                    font.family: root.uiFont
                    font.pixelSize: 14
                    clip: true
                    echoMode: TextInput.Password
                    passwordCharacter: "•"
                    selectByMouse: true
                    KeyNavigation.tab: loginButton
                    KeyNavigation.backtab: userField
                    Keys.onReturnPressed: root.tryLogin()
                    Keys.onEnterPressed: root.tryLogin()
                }
            }

            Rectangle {
                id: loginButton
                width: parent.width
                height: 42
                radius: 8
                color: loginArea.containsMouse ? "#c44b44" : root.accent
                Accessible.role: Accessible.Button
                Accessible.name: "Log in"
                Keys.onReturnPressed: root.tryLogin()
                Keys.onEnterPressed: root.tryLogin()
                Keys.onSpacePressed: root.tryLogin()
                KeyNavigation.tab: rebootText
                KeyNavigation.backtab: passwordField
                focus: false

                Text {
                    anchors.centerIn: parent
                    color: "#fff8f6"
                    font.family: root.uiFont
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    text: "Log in"
                }
                MouseArea {
                    id: loginArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.tryLogin()
                }
            }

            Text {
                id: message
                width: parent.width
                color: "#e09088"
                font.family: root.uiFont
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: text.length > 0
            }

            Text {
                id: loadError
                width: parent.width
                color: "#e09088"
                font.family: root.uiFont
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                visible: __sddm_errors !== ""
                text: __sddm_errors
            }
        }
    }

    Row {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        spacing: 18

        Loader {
            id: batteryStatus
            source: "BatteryStatus.qml"
            asynchronous: true
            width: item && item.visible ? item.implicitWidth : 0
            height: item && item.visible ? item.implicitHeight : 0
        }

        Text {
            id: rebootText
            visible: sddm.canReboot
            color: rebootArea.containsMouse ? "#e8eaed" : "#6e747e"
            font.family: root.uiFont
            font.pixelSize: 12
            text: "Restart"
            KeyNavigation.tab: shutdownText
            KeyNavigation.backtab: loginButton
            MouseArea {
                id: rebootArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sddm.reboot()
            }
        }

        Text {
            id: shutdownText
            visible: sddm.canPowerOff
            color: shutdownArea.containsMouse ? "#e8eaed" : "#6e747e"
            font.family: root.uiFont
            font.pixelSize: 12
            text: "Shut down"
            KeyNavigation.backtab: rebootText
            MouseArea {
                id: shutdownArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: sddm.powerOff()
            }
        }
    }

    Component.onCompleted: {
        if (userField.text.length > 0)
            passwordField.forceActiveFocus()
        else
            userField.forceActiveFocus()
    }
}
