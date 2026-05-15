import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import MyPomodoro 1.0
import QtMultimedia 6.2
import QtQuick.Effects


Page {
    id: pomodoroView

    anchors.fill: parent

    FontLoader {
        id: fredoka
        source: "../assets/fonts/Fredoka.ttf"
    }

    Material.theme: Material.Light
    Material.primary: Material.Amber
    Material.accent: Material.Yellow

    function formatTime(seconds) {
        let m = Math.floor(seconds / 60);
        let s = seconds % 60;
        let mm = m < 10 ? "0" + m : m;
        let ss = s < 10 ? "0" + s : s;
        return mm + ":" + ss;
    }

    background: Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#FFDE59" }
            GradientStop { position: 1.0; color: "#FFF8DC" }
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 32
        width: parent.width * 0.55

        Label {
            text: "Pomodoro Timer"
            font.pixelSize: 60
            //font.weight: Font.Medium
            font.weight: Font.Bold
            color: "#333333"
            Layout.alignment: Qt.AlignHCenter
            font.family: fredoka.name

        }

        // Live Countdown Display - Material Card
        Pane {
            Layout.preferredWidth: parent.width * 0.65
            Layout.preferredHeight: 140
            Layout.alignment: Qt.AlignHCenter
            Material.elevation: 6
            Material.roundedScale: Material.MediumScale

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }


            background: Rectangle {
                color: "white"
                radius: 70
                // border.color: "#333333"
                // border.width: 2
            }

            Label {
                id: timerText
                anchors.centerIn: parent
                font.pixelSize: 72
                font.weight: Font.Medium
                text: pomodoroView.formatTime(pomodoroTimer.timeRemaining)
                color: "#333333"
                font.family: fredoka.name

            }
        }

        Connections {
            target: pomodoroTimer
            function onTimeRemainingChanged() {
                timerText.text = pomodoroView.formatTime(pomodoroTimer.timeRemaining)
            }

            function onSessionEnded() {
                console.log("Pomodoro session ended!");
            }

            function onClosePomodorotimer() {
                console.log("received closed signal");
                if(pomodoroView.StackView.view){
                    pomodoroView.StackView.view.pop();
                }
            }
        }

        // Status indicator with Material Label
        Label {
            id: statusText
            text: {
                switch(pomodoroTimer.currentState) {
                    case 0: return "Ready to start";
                    case 1: return "Focus time!";
                    case 2: return "Break time!";
                    default: return "";
                }
            }
            font.pixelSize: 24
            font.weight: Font.Normal
            color: Material.foreground
            Layout.alignment: Qt.AlignHCenter
            font.family: fredoka.name

        }

        // Primary action buttons
        RowLayout {
            spacing: 16
            Layout.alignment: Qt.AlignHCenter

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            Button {
                text: "Start"
                font.pixelSize: 16
                Material.elevation: 2
                Material.background: "white"
                Material.foreground: "white"
                highlighted: true
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                onClicked: pomodoroTimer.startSession(25*60, 5*60)
                font.family: fredoka.name
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#50000000"
                    shadowBlur: 1.5
                    shadowHorizontalOffset: 0
                    shadowVerticalOffset: 6
                }
            }



            Button {
                text: "Stop"
                font.pixelSize: 16
                Material.elevation: 2
                Material.background: "white"
                flat: false
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                onClicked: pomodoroTimer.stop()
                font.family: fredoka.name
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#50000000"
                    shadowBlur: 1.5
                    shadowHorizontalOffset: 0
                    shadowVerticalOffset: 6
                }

            }
        }

        // Secondary action buttons
        RowLayout {
            spacing: 16
            Layout.alignment: Qt.AlignHCenter

            Button {
                text: "Pause"
                font.pixelSize: 16
                flat: true
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                Material.background: "white"
                onClicked: pomodoroTimer.pause()
                font.family: fredoka.name
                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#50000000"
                    shadowBlur: 1.5
                    shadowHorizontalOffset: 0
                    shadowVerticalOffset: 6
                }

            }

            Button {
                text: "Resume"
                font.pixelSize: 16
                flat: true
                Layout.preferredWidth: 120
                Layout.preferredHeight: 48
                Material.background: "white"
                onClicked: pomodoroTimer.resume()
                font.family: fredoka.name

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: "#50000000"
                    shadowBlur: 1.5
                    shadowHorizontalOffset: 0
                    shadowVerticalOffset: 6
                }

            }
        }

        // Back button with spacing
        Item {
            Layout.preferredHeight: 16
        }

        Button {
            text: "Back to Menu"
            font.pixelSize: 16
            flat: true
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 160
            Material.background: "white"
            Layout.preferredHeight: 48
            Layout.topMargin: -50
            onClicked: {
                console.log("Back to Menu clicked")
                pomodoroTimer.killPomodoroView()
            }
            font.family: fredoka.name

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

        }
    }
}
