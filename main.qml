import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Controls.Material 2.13
import QtQuick.Window 2.13
import QtMultimedia 5.13

import org.pkoretic.voicetranslator 1.0

ApplicationWindow {
    visible: true
    width: 1920
    height: 1080
    title: qsTr("Translator")
    visibility: Window.FullScreen

    // theme settings
    Material.theme: Material.Dark
    Material.accent: Material.BlueGrey

    VoiceTranslator { id: translator }

    Column {
        anchors.centerIn: parent

        Label {
            id: txInfo
            text: qsTr("Press OK to say command")
            font.italic: true
            anchors.horizontalCenter: parent.horizontalCenter

            opacity: translator.running ? 0 : 1
            Behavior on opacity { NumberAnimation {} }
        }

        Button {
            id: btTranslate
            text: qsTr("OK")
            icon.name: "audio-input-microphone"
            anchors.horizontalCenter: parent.horizontalCenter
            Keys.onEnterPressed: clicked()
            Keys.onReturnPressed: clicked()
            onClicked: translator.start()
            enabled: !translator.running
            focus: true

            opacity: translator.running ? 0 : 1
            Behavior on opacity { NumberAnimation {} }
        }

        BusyIndicator {
            id: busyIndicator
            running: translator.running
            anchors.horizontalCenter: parent.horizontalCenter

            opacity: translator.running ? 1 : 0
            Behavior on opacity { NumberAnimation {} }
        }

        Label {
            id: lCommand
            text: translator.command
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter

            opacity: translator.command.length > 0 && translator.running ? 0 : 1
            Behavior on opacity { NumberAnimation {} }
        }

        Label {
            id: lError
            text: translator.error
            color: "red"
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
