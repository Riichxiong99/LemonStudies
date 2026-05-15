import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtQuick.Effects
import MyPomodoro 1.0


Page {
    id: todoView
    anchors.fill: parent

    FontLoader {
        id: fredoka
        source: "../assets/fonts/Fredoka.ttf"
    }

    // Define the property and immediately access the context property
    property var todoModel: todoModelInstance

    // Connection to handle the close signal
    Connections {
        target: todoModel
        function onCloseTodoView() {
            if (todoView.StackView.view) {
                todoView.StackView.view.pop()
            }
        }
    }


    // Background
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#FFDE59" }
            GradientStop { position: 1.0; color: "#FFF8DC" }
        }
    }


    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Header Stats Card
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 15
            // color: "white"
            // border.color: "black"

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            MultiEffect {
                anchors.fill: parent
                source: parent
                shadowEnabled: true
                shadowColor: "#20000000"
                shadowBlur: 0.4
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 2
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 20

                Column {
                    Layout.fillWidth: true

                    Label {
                        text: "Total Tasks"
                        font.pixelSize: 14
                        color: "#666"
                        font.family: fredoka.name
                    }
                    Label {
                        text: todoModel.rowCount()
                        font.pixelSize: 28
                        font.bold: true
                        color: "#2c3e50"
                        font.family: fredoka.name
                    }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#e0e0e0"
                }

                Column {
                    Layout.fillWidth: true

                    Label {
                        text: "Completed"
                        font.pixelSize: 14
                        color: "#666"
                        font.family: fredoka.name

                    }
                    Label {
                        text: {
                            let completed = 0;
                            for (let i = 0; i < todoModel.rowCount(); i++) {
                                if (todoModel.data(todoModel.index(i, 0), 259)) {
                                }
                            }
                            return completed;
                        }
                        font.pixelSize: 28
                        font.bold: true
                        color: "#27ae60"
                    }
                }

                Rectangle {
                    width: 1
                    Layout.fillHeight: true
                    color: "#e0e0e0"
                }

                Button {
                    id: backButton
                    text: "←"
                    flat: true
                    Material.foreground: "grey"
                    Layout.preferredWidth: 80
                    font.family: fredoka.name


                    ToolTip.visible: hovered
                    ToolTip.text: "Go back"

                    onClicked: {
                        console.log("Back button clicked")
                        todoModel.killTodoView()
                    }
                }
            }
        }

        // Add new task section
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            radius: 15
            color: "white"

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            MultiEffect {
                anchors.fill: parent
                source: parent
                shadowEnabled: true
                shadowColor: "#20000000"
                shadowBlur: 0.4
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 2
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                TextField {
                    id: newTaskInput
                    Layout.fillWidth: true
                    placeholderText: "Enter new task..."
                    font.pixelSize: 14
                    Material.accent: "#333333"
                    font.family: fredoka.name


                    onAccepted: {
                        if (text.trim() !== "") {
                            todoModel.addTodo(text, "", new Date(), 1)
                            text = ""
                        }
                    }
                }

                Button {
                    text: "Add Task"
                    font.family: fredoka.name
                    enabled: newTaskInput.text.trim() !== ""
                    Material.background: "#FFDE59"
                    Material.foreground: "#333333"
                    onClicked: {
                        if (newTaskInput.text.trim() !== "") {
                            todoModel.addTodo(newTaskInput.text, "", new Date(), 1)
                            newTaskInput.text = ""
                        }
                    }

                }
            }
        }

        // Filter controls
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            color: "white"
            radius: 15

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                ComboBox {
                    id: priorityFilter
                    model: ["All Priorities", "High", "Medium", "Low"]
                    Layout.preferredWidth: 150
                    font.family: fredoka.name
                    Material.accent: "#333333"

                }

                CheckBox {
                    id: hideCompleted
                    text: "Hide Completed"
                    font.family: fredoka.name
                    Material.accent: "#FFDE59"

                }

                Item { Layout.fillWidth: true }

                Button {
                    text: "Clear Completed"
                    Material.background: "#FFDE59"
                    Material.foreground: "#333333"

                    ToolTip.visible: hovered
                    ToolTip.text: "Remove all completed tasks"
                    font.family: fredoka.name
                    onClicked: todoModel.clearCompleted()
                }
            }
        }

        // Todo list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#50000000"
                shadowBlur: 1.5
                shadowHorizontalOffset: 0
                shadowVerticalOffset: 6
            }

            ListView {
                id: todoList
                model: todoModel
                spacing: 10

                delegate: Rectangle {
                    width: todoList.width
                    height: 100
                    color: "white"
                    radius: 15

                    // Border for completed tasks
                    border.color: model.completed ? "#27ae60" : "transparent"
                    border.width: 2

                    // Filter logic
                    visible: {
                        if (hideCompleted.checked && model.completed) return false;

                        if (priorityFilter.currentIndex > 0) {
                            let filterPriority;
                            switch (priorityFilter.currentIndex) {
                                case 1: filterPriority = 2; break; // High
                                case 2: filterPriority = 1; break; // Medium
                                case 3: filterPriority = 0; break; // Low
                            }
                            if (model.priority !== filterPriority) return false;
                        }

                        return true;
                    }

                    // Shadow effect
                    MultiEffect {
                        anchors.fill: parent
                        source: parent
                        shadowEnabled: true
                        shadowColor: "#20000000"
                        shadowBlur: 0.4
                        shadowHorizontalOffset: 0
                        shadowVerticalOffset: 2
                    }

                    // Highlight for completed
                    Rectangle {
                        anchors.fill: parent
                        radius: 15
                        color: model.completed ? "#27ae60" : "transparent"
                        opacity: 0.1
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 15
                        spacing: 15

                        // Priority indicator circle
                        Rectangle {
                            width: 50
                            height: 50
                            radius: 25
                            color: {
                                if (model.completed) return "#27ae60"
                                switch(model.priority) {
                                    case 2: return "#e74c3c"; // High
                                    case 1: return "#f39c12"; // Medium
                                    case 0: return "#95a5a6"; // Low
                                    default: return "#95a5a6";
                                }
                            }

                            Label {
                                anchors.centerIn: parent
                                text: {
                                    if (model.completed) {
                                        return "✓"
                                    }
                                    switch(model.priority) {
                                        case 2: return "!!"
                                        case 1: return "!"
                                        case 0: return "·"
                                        default: return "·"
                                    }
                                }
                                font.family: fredoka.name
                                font.pixelSize: model.completed ? 24 : 20
                                font.bold: true
                                color: "white"
                            }
                        }

                        // Task Info
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 5

                            Label {
                                text: model.title
                                font.pixelSize: 18
                                font.bold: true
                                font.strikeout: model.completed
                                color: model.completed ? "#7f8c8d" : "#2c3e50"
                                font.family: fredoka.name

                            }

                            Label {
                                text: model.description || "No description"
                                font.pixelSize: 13
                                color: "#7f8c8d"
                                elide: Text.ElideRight
                                visible: model.description !== ""
                                font.family: fredoka.name

                            }

                            RowLayout {
                                spacing: 15

                                Label {
                                    text: {
                                        if (!model.dueDate) return "";
                                        let date = new Date(model.dueDate);
                                        return "Due: " + date.toLocaleDateString();
                                    }
                                    font.pixelSize: 13
                                    color: {
                                        if (!model.dueDate) return "#7f8c8d";
                                        let now = new Date();
                                        let due = new Date(model.dueDate);
                                        return due < now && !model.completed ? "#e74c3c" : "#7f8c8d";
                                    }
                                    visible: model.dueDate !== undefined
                                    font.family: fredoka.name

                                }

                                Label {
                                    text: {
                                        switch(model.priority) {
                                            case 2: return "High Priority"
                                            case 1: return "Medium Priority"
                                            case 0: return "Low Priority"
                                            default: return ""
                                        }
                                    }
                                    font.pixelSize: 12
                                    color: {
                                        switch(model.priority) {
                                            case 2: return "#e74c3c"
                                            case 1: return "#f39c12"
                                            case 0: return "#27ae60"
                                            default: return "#7f8c8d"
                                        }
                                    }
                                    font.family: fredoka.name

                                }
                            }
                        }

                        // Action Buttons
                        RowLayout {
                            spacing: 5
                            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                            Button {
                                text: model.completed ? "↻" : "✓"
                                Material.background: model.completed ? Material.Orange : Material.Green
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: model.completed ? "Mark as incomplete" : "Mark as complete"

                                onClicked: {
                                    todoModel.markAsCompleted(index, !model.completed)
                                }
                                font.family: fredoka.name

                            }

                            Button {
                                text: "✎"
                                Material.background: Material.Blue
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: "Edit task"

                                onClicked: editDialog.open(index, model.title, model.description, model.dueDate, model.priority)
                                font.family: fredoka.name

                            }

                            Button {
                                text: "X"
                                Material.background: Material.Red
                                Material.foreground: "white"
                                Layout.preferredWidth: 50

                                ToolTip.visible: hovered
                                ToolTip.text: "Delete task"

                                onClicked: {
                                    deleteDialog.taskIndex = index
                                    deleteDialog.taskTitle = model.title
                                    deleteDialog.open()
                                }
                                font.family: fredoka.name

                            }
                        }
                    }
                }
            }
        }
    }

    // Delete Confirmation Dialog
    Dialog {
        id: deleteDialog
        property int taskIndex: -1
        property string taskTitle: ""
        Material.accent: "#333333"

        title: "Delete Task?"
        standardButtons: Dialog.Yes | Dialog.No
        anchors.centerIn: parent

        Label {
            text: "Are you sure you want to delete '" + deleteDialog.taskTitle + "'?\nThis action cannot be undone."
        }

        onAccepted: {
            if (taskIndex >= 0) {
                todoModel.removeTodo(taskIndex)
            }
        }
        font.family: fredoka.name

    }

    // Dialog for editing tasks
    Dialog {
        id: editDialog
        title: "Edit Task"
        standardButtons: Dialog.Save | Dialog.Cancel
        width: Math.min(todoView.width * 0.75, 550)
        height: Math.min(todoView.height * 0.95, 600)
        anchors.centerIn: Overlay.overlay
        modal: true
        font.family: fredoka.name
        Material.accent: "#333333"

        property int currentIndex: -1
        property date selectedDate: new Date()

        function open(index, title, description, dueDate, priority) {
            currentIndex = index;
            editTitle.text = title;
            editDescription.text = description || "";
            editPriority.currentIndex = priority;

            if (dueDate) {
                let date = new Date(dueDate);
                selectedDate = date;

                // Set the values in the date pickers
                dayCombo.currentIndex = date.getDate() - 1;
                monthCombo.currentIndex = date.getMonth();
                yearField.text = date.getFullYear().toString();
            } else {
                let today = new Date();
                selectedDate = today;

                dayCombo.currentIndex = today.getDate() - 1;
                monthCombo.currentIndex = today.getMonth();
                yearField.text = today.getFullYear().toString();
            }

            visible = true;
        }

        contentItem: ScrollView {

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 15

                Label {
                    text: "Title:"
                    font.bold: true
                    font.family: fredoka.name

                }

                TextField {
                    id: editTitle
                    Layout.fillWidth: true
                    placeholderText: "Task title"
                    font.family: fredoka.name

                }

                Label {
                    text: "Description:"
                    font.bold: true
                    font.family: fredoka.name

                }

                TextArea {
                    id: editDescription
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    placeholderText: "Task description"
                    wrapMode: TextEdit.Wrap
                    font.family: fredoka.name
                    onTextChanged: {
                        var cursor = cursorPosition
                        var cleanText = text

                        // Remove all newlines
                        cleanText = cleanText.replace(/\n/g, ' ')

                        // Limit to 100 characters
                        if (cleanText.length > 100) {
                            cleanText = cleanText.substring(0, 100)
                        }

                        if (cleanText !== text) {
                            text = cleanText
                            cursorPosition = Math.min(cursor, cleanText.length)
                        }
                        }

                }

                Label {
                    text: "Due Date:"
                    font.bold: true
                    font.family: fredoka.name

                }

                // Custom date selector using basic components
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    ComboBox {
                        id: dayCombo
                        model: {
                            let days = [];
                            for (let i = 1; i <= 31; i++) {
                                days.push(i);
                            }
                            return days;
                        }
                        Layout.preferredWidth: 80

                        onCurrentValueChanged: {
                            editDialog.updateSelectedDate();
                        }
                    }

                    ComboBox {
                        id: monthCombo
                        model: ["January", "February", "March", "April", "May", "June", "July",
                               "August", "September", "October", "November", "December"]
                        Layout.preferredWidth: 120
                        font.family: fredoka.name
                        onCurrentIndexChanged: {
                            // Adjust days in month based on selected month
                            let year = parseInt(yearField.text) || new Date().getFullYear();
                            let daysInMonth = new Date(year, currentIndex + 1, 0).getDate();

                            // Keep current day if valid for new month, otherwise set to last day of month
                            if (dayCombo.currentIndex >= daysInMonth) {
                                dayCombo.currentIndex = daysInMonth - 1;
                            }

                            editDialog.updateSelectedDate();
                        }
                    }

                    TextField {
                        id: yearField
                        placeholderText: "Year"
                        inputMethodHints: Qt.ImhDigitsOnly
                        Layout.preferredWidth: 80
                        validator: IntValidator { bottom: 2000; top: 2100 }

                        onTextChanged: {
                            if (text.length === 4) {
                                editDialog.updateSelectedDate();
                            }
                        }
                        font.family: fredoka.name

                    }
                }

                Label {
                    text: "Priority:"
                    font.bold: true
                    font.family: fredoka.name

                }

                ComboBox {
                    id: editPriority
                    Layout.fillWidth: true
                    model: ["Low", "Medium", "High"]
                }
            }
        }

        // Function to update the selected date based on the fields
        function updateSelectedDate() {
            let day = dayCombo.currentValue;
            let month = monthCombo.currentIndex;
            let year = parseInt(yearField.text);

            if (!isNaN(year) && year >= 2000 && year <= 2100) {
                selectedDate = new Date(year, month, day);
            }
        }

        onAccepted: {
            if (editTitle.text.trim() === "") return;
            updateSelectedDate();

            todoModel.updateTodo(
                currentIndex,
                editTitle.text,
                editDescription.text,
                selectedDate,
                editPriority.currentIndex
            );
        }
    }

    // Save/Load functionality
    Component.onDestruction: {
        todoModel.saveTodos("todos.json")
    }
}
