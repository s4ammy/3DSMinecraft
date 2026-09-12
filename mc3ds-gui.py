from __future__ import annotations

import sys
from pathlib import Path

try:
    from PySide6.QtCore import QProcess, Qt, QUrl
    from PySide6.QtGui import QDesktopServices, QFontDatabase, QTextCursor
    from PySide6.QtWidgets import (
        QApplication,
        QCheckBox,
        QComboBox,
        QFileDialog,
        QFrame,
        QHBoxLayout,
        QLabel,
        QLineEdit,
        QMainWindow,
        QMessageBox,
        QPlainTextEdit,
        QProgressBar,
        QPushButton,
        QScrollArea,
        QStackedWidget,
        QVBoxLayout,
        QWidget,
    )
except ImportError as error:
    raise SystemExit("This GUI needs PySide6. Install it with: python -m pip install -r gui-requirements.txt") from error


ROOT = Path(__file__).resolve().parent
ACCENT = "#babaff"
OK = "#8fdcb0"
DANGER = "#e89090"
TEXT_DIM = "#ababab"

STYLE = """
QWidget {
    background: transparent;
    color: #e8e8f2;
    font-family: "IBM Plex Sans";
    font-size: 13px;
}
QMainWindow, QWidget#Root, QWidget#PageBody, QScrollArea#PageScroll {
    background: #1e1e1e;
}
QWidget#Sidebar {
    background: #141414;
    border-right: 1px solid #2f2f2f;
}
QLabel#Brand {
    font-family: "Chakra Petch";
    font-size: 18px;
    font-weight: 600;
}
QLabel#BrandSub, QLabel#PageSub, QLabel#Eyebrow, QLabel#FooterHint {
    font-family: "IBM Plex Mono";
    color: #727272;
    font-size: 11px;
}
QLabel#PageTitle {
    font-family: "Chakra Petch";
    font-size: 25px;
    font-weight: 600;
}
QLabel#CardTitle {
    font-family: "Chakra Petch";
    font-size: 16px;
    font-weight: 500;
}
QLabel#HelpText {
    color: #ababab;
    font-size: 12px;
}
QFrame[role="card"] {
    background: #141414;
    border: 1px solid #2f2f2f;
    border-radius: 2px;
}
QFrame[role="note"] {
    background: #25253a;
    border: 1px solid #4e4e7f;
    border-radius: 2px;
}
QPushButton[role="nav"] {
    text-align: left;
    padding: 10px 14px;
    color: #ababab;
    border: none;
    border-radius: 2px;
    background: transparent;
}
QPushButton[role="nav"]:hover, QPushButton[role="nav"][active="true"] {
    color: #babaff;
    background: #4e4e7f;
}
QPushButton[role="primary"] {
    color: #141414;
    background: #babaff;
    border: none;
    border-radius: 2px;
    font-weight: 600;
    padding: 9px 18px;
    min-height: 26px;
}
QPushButton[role="primary"]:hover {
    background: #c9c9ff;
}
QPushButton[role="primary"]:disabled {
    color: #727272;
    background: #2f2f2f;
}
QPushButton[role="ghost"] {
    color: #ababab;
    background: transparent;
    border: 1px solid #4a4a4a;
    border-radius: 2px;
    padding: 7px 13px;
    min-height: 24px;
}
QPushButton[role="ghost"]:hover {
    color: #e8e8f2;
    border-color: #babaff;
}
QPushButton[role="ghost"]:disabled {
    color: #727272;
    border-color: #2f2f2f;
}
QLineEdit, QComboBox, QPlainTextEdit {
    background: #0a0a0a;
    color: #e8e8f2;
    border: 1px solid #4a4a4a;
    border-radius: 2px;
    padding: 7px 10px;
    selection-background-color: #4e4e7f;
}
QLineEdit:focus, QComboBox:focus {
    border-color: #babaff;
}
QLineEdit:disabled, QComboBox:disabled {
    color: #727272;
    border-color: #2f2f2f;
}
QComboBox::drop-down {
    border: none;
    width: 22px;
}
QComboBox QAbstractItemView {
    background: #141414;
    color: #e8e8f2;
    selection-background-color: #4e4e7f;
}
QPlainTextEdit {
    font-family: "IBM Plex Mono";
    font-size: 11px;
    padding: 10px;
}
QCheckBox {
    spacing: 8px;
    color: #ababab;
}
QProgressBar {
    background: #0a0a0a;
    border: 1px solid #2f2f2f;
    border-radius: 2px;
    min-height: 6px;
    max-height: 6px;
    text-align: center;
}
QProgressBar::chunk {
    background: #babaff;
}
QScrollArea {
    border: none;
}
QScrollBar:vertical {
    background: #141414;
    width: 8px;
}
QScrollBar::handle:vertical {
    background: #4a4a4a;
    min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0px;
}
"""


def LoadFonts() -> None:
    fontDirectory = ROOT / "assets" / "fonts"
    for fontPath in fontDirectory.glob("*.ttf"):
        QFontDatabase.addApplicationFont(str(fontPath))


def FindPatcher(bootstrapMode: bool, checkOnly: bool) -> Path:
    executable = "mc3ds-patcher.exe" if sys.platform == "win32" else "mc3ds-patcher"
    ctrtool = "ctrtool.exe" if sys.platform == "win32" else "ctrtool"
    makerom = "makerom.exe" if sys.platform == "win32" else "makerom"
    candidates = [
        ROOT / executable,
        ROOT / "build" / executable,
        ROOT / "build" / "Release" / executable,
        ROOT / "build-no-tests" / executable,
    ]
    for candidate in candidates:
        toolsDirectory = candidate.parent / "tools"
        if candidate.is_file() and (toolsDirectory / ctrtool).is_file() and (
            not bootstrapMode or checkOnly or (toolsDirectory / makerom).is_file()
        ):
            return candidate

    raise ValueError("The patcher or its tools are missing. Keep the executable and tools folder beside this GUI, or build with MC3DS_FETCH_TOOLS=ON.")


def MakeCard(title: str) -> tuple[QFrame, QVBoxLayout]:
    card = QFrame()
    card.setProperty("role", "card")
    layout = QVBoxLayout(card)
    layout.setContentsMargins(20, 17, 20, 18)
    layout.setSpacing(12)
    heading = QLabel(title)
    heading.setObjectName("CardTitle")
    layout.addWidget(heading)
    return card, layout


class CPathField(QWidget):
    def __init__(self, label: str, directory: bool = False, fileFilter: str = "All files (*)") -> None:
        super().__init__()
        self.directory = directory
        self.fileFilter = fileFilter
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(6)
        layout.addWidget(QLabel(label))
        row = QHBoxLayout()
        row.setSpacing(8)
        self.lineEdit = QLineEdit()
        self.lineEdit.setPlaceholderText("Choose a folder" if directory else "Choose a file")
        row.addWidget(self.lineEdit, 1)
        browseButton = QPushButton("Browse")
        browseButton.setProperty("role", "ghost")
        browseButton.clicked.connect(self.browse)
        row.addWidget(browseButton)
        layout.addLayout(row)

    def path(self) -> str:
        return self.lineEdit.text().strip()

    def browse(self) -> None:
        start = self.path() or str(Path.home())
        if self.directory:
            selected = QFileDialog.getExistingDirectory(self, "Choose output folder", start)
        else:
            selected, _ = QFileDialog.getOpenFileName(self, "Choose file", start, self.fileFilter)

        if selected:
            self.lineEdit.setText(selected)


class CFormPage(QWidget):
    def __init__(self, bootstrapMode: bool) -> None:
        super().__init__()
        self.bootstrapMode = bootstrapMode
        outer = QVBoxLayout(self)
        outer.setContentsMargins(0, 0, 0, 0)
        outer.setSpacing(0)
        scroll = QScrollArea()
        scroll.setObjectName("PageScroll")
        scroll.setWidgetResizable(True)
        outer.addWidget(scroll)
        body = QWidget()
        body.setObjectName("PageBody")
        scroll.setWidget(body)
        layout = QVBoxLayout(body)
        layout.setContentsMargins(34, 28, 34, 30)
        layout.setSpacing(18)

        eyebrow = QLabel("ONE-TIME SETUP" if bootstrapMode else "EXECUTABLE UPDATE")
        eyebrow.setObjectName("Eyebrow")
        layout.addWidget(eyebrow)
        title = QLabel("First-time setup" if bootstrapMode else "Make a Luma patch")
        title.setObjectName("PageTitle")
        layout.addWidget(title)
        subtitle = QLabel(
            "Build the base CIA, then the update CIA if you use it. Install them once with FBI."
            if bootstrapMode else
            "After setup, replace code.ips to try a new patch. No CIA reinstall is needed."
        )
        subtitle.setObjectName("PageSub")
        subtitle.setWordWrap(True)
        layout.addWidget(subtitle)

        note = QFrame()
        note.setProperty("role", "note")
        noteLayout = QVBoxLayout(note)
        noteLayout.setContentsMargins(15, 11, 15, 11)
        noteLabel = QLabel(
            "Use your own original European CIA files. Back up saves before installing."
            if bootstrapMode else
            "Use the original CIA that matches the installed update. Keep the bootstrap titles installed."
        )
        noteLabel.setWordWrap(True)
        noteLayout.addWidget(noteLabel)
        layout.addWidget(note)

        sourceCard, sourceLayout = MakeCard("Game files")
        if bootstrapMode:
            self.baseField = CPathField("Original base CIA", fileFilter="CIA files (*.cia);;All files (*)")
            self.updateField = CPathField("Original update CIA (optional)", fileFilter="CIA files (*.cia);;All files (*)")
            self.updateField.lineEdit.textChanged.connect(self.updateOptions)
            sourceLayout.addWidget(self.baseField)
            sourceLayout.addWidget(self.updateField)
        else:
            self.inputType = QComboBox()
            self.inputType.addItems(["Update installed", "Base game only"])
            self.inputType.currentIndexChanged.connect(self.updateOptions)
            sourceLayout.addWidget(QLabel("What is installed on your 3DS?"))
            sourceLayout.addWidget(self.inputType)
            self.inputField = CPathField("Original update CIA", fileFilter="CIA files (*.cia);;All files (*)")
            sourceLayout.addWidget(self.inputField)

        sourceLayout.addWidget(QLabel("Base-game seed"))
        self.seedType = QComboBox()
        self.seedType.addItems(["Raw 16-byte seed", "Seed database", "No seed (decrypted CIA)"])
        self.seedType.currentIndexChanged.connect(self.updateOptions)
        sourceLayout.addWidget(self.seedType)
        self.seedField = CPathField("Seed file", fileFilter="Seed files (*.bin);;All files (*)")
        sourceLayout.addWidget(self.seedField)
        helpText = QLabel("The original base CIA usually needs a seed. The reference update does not.")
        helpText.setObjectName("HelpText")
        helpText.setWordWrap(True)
        sourceLayout.addWidget(helpText)
        layout.addWidget(sourceCard)

        optionsCard, optionsLayout = MakeCard("Patch options")
        optionsLayout.addWidget(QLabel("Controls"))
        self.controls = QComboBox()
        self.controls.addItem("L + Circle Pad", "l-circle-pad")
        self.controls.addItem("Circle Pad Pro", "circle-pad-pro")
        optionsLayout.addWidget(self.controls)
        self.overlay = QCheckBox("Show the FPS/debug overlay (update only)")
        optionsLayout.addWidget(self.overlay)
        self.checkOnly = QCheckBox("Check inputs only, without writing files")
        optionsLayout.addWidget(self.checkOnly)
        layout.addWidget(optionsCard)

        outputCard, outputLayout = MakeCard("Output")
        label = "Setup folder" if bootstrapMode else "Patch folder"
        self.outputField = CPathField(label, directory=True)
        self.outputField.lineEdit.setText(str(Path.home() / ("Minecraft-old3ds-setup" if bootstrapMode else "Minecraft-luma")))
        outputLayout.addWidget(self.outputField)
        outputHint = QLabel(
            "The setup folder will hold bootstrap-base and, if selected, bootstrap-update."
            if bootstrapMode else
            "code.ips and report.txt will be written here. Existing bootstrap.cia files are left alone."
        )
        outputHint.setObjectName("HelpText")
        outputHint.setWordWrap(True)
        outputLayout.addWidget(outputHint)
        layout.addWidget(outputCard)

        self.runButton = QPushButton("Build bootstrap CIAs" if bootstrapMode else "Make code.ips")
        self.runButton.setProperty("role", "primary")
        layout.addWidget(self.runButton, 0, Qt.AlignRight)
        layout.addStretch(1)
        self.updateOptions()

    def updateOptions(self) -> None:
        updateSelected = bool(self.updateField.path()) if self.bootstrapMode else self.inputType.currentIndex() == 0
        self.overlay.setEnabled(updateSelected)
        if not updateSelected:
            self.overlay.setChecked(False)

        baseNeeded = self.bootstrapMode or not updateSelected
        self.seedType.setEnabled(baseNeeded)
        self.seedField.setEnabled(baseNeeded and self.seedType.currentIndex() != 2)
        if not self.bootstrapMode:
            self.inputField.findChild(QLabel).setText("Original update CIA" if updateSelected else "Original base CIA")

    def buildTasks(self) -> tuple[list[tuple[str, list[str]]], Path, bool]:
        outputText = self.outputField.path()
        if not outputText:
            raise ValueError("Choose an output folder.")

        outputPath = Path(outputText).expanduser().resolve()
        if outputPath == outputPath.parent or (outputPath.exists() and not outputPath.is_dir()):
            raise ValueError("Choose a normal output folder, not a filesystem root or file.")

        checkOnly = self.checkOnly.isChecked()
        controls = ["--controls", str(self.controls.currentData())]
        seed = []
        baseNeeded = self.bootstrapMode or self.inputType.currentIndex() == 1
        if baseNeeded and self.seedType.currentIndex() != 2:
            seedText = self.seedField.path()
            if not seedText:
                raise ValueError("Choose a base-game seed file, or select 'No seed' for a decrypted CIA.")

            seedPath = Path(seedText).expanduser().resolve()
            if not seedPath.is_file():
                raise ValueError("The seed file does not exist.")

            seed = ["--seed-file" if self.seedType.currentIndex() == 0 else "--seeddb", str(seedPath)]

        def checkedCia(field: CPathField, name: str) -> Path:
            path = Path(field.path()).expanduser().resolve()
            if not field.path() or not path.is_file() or path.suffix.lower() != ".cia":
                raise ValueError(f"Choose your original {name} CIA file.")

            return path

        tasks = []
        if self.bootstrapMode:
            basePath = checkedCia(self.baseField, "base")
            baseOutput = outputPath / "bootstrap-base"
            tasks.append(("base bootstrap CIA", [str(basePath), *seed, *controls, "--bootstrap-cia", "-o", str(baseOutput)]))
            if self.updateField.path():
                updatePath = checkedCia(self.updateField, "update")
                updateOutput = outputPath / "bootstrap-update"
                updateArguments = [str(updatePath), *controls, "--bootstrap-cia", "-o", str(updateOutput)]
                if self.overlay.isChecked():
                    updateArguments.append("--overlay")

                tasks.append(("update bootstrap CIA", updateArguments))

            if not checkOnly:
                for _, arguments in tasks:
                    if (Path(arguments[arguments.index("-o") + 1]) / "bootstrap.cia").exists():
                        raise ValueError("A bootstrap.cia already exists in the output. Choose a new setup folder.")
        else:
            updateSelected = self.inputType.currentIndex() == 0
            inputPath = checkedCia(self.inputField, "update" if updateSelected else "base")
            arguments = [str(inputPath), *controls, "-o", str(outputPath)]
            if not updateSelected:
                arguments.extend(seed)
            elif self.overlay.isChecked():
                arguments.append("--overlay")

            tasks.append(("Luma patch", arguments))

        if checkOnly:
            for _, arguments in tasks:
                arguments.append("--dry-run")

        return tasks, outputPath, checkOnly


class CMainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("Minecraft Old 3DS Patcher")
        self.resize(1060, 760)
        self.setMinimumSize(860, 620)
        self.tasks: list[tuple[str, list[str]]] = []
        self.patcherPath = Path()
        self.outputPath = Path()
        self.checkOnly = False
        self.bootstrapRun = False
        self.hasUpdate = False
        self.busy = False

        root = QWidget()
        root.setObjectName("Root")
        self.setCentralWidget(root)
        shell = QHBoxLayout(root)
        shell.setContentsMargins(0, 0, 0, 0)
        shell.setSpacing(0)

        sidebar = QWidget()
        sidebar.setObjectName("Sidebar")
        sidebar.setFixedWidth(214)
        sideLayout = QVBoxLayout(sidebar)
        sideLayout.setContentsMargins(18, 26, 18, 20)
        sideLayout.setSpacing(7)
        brand = QLabel("Minecraft 3DS")
        brand.setObjectName("Brand")
        sideLayout.addWidget(brand)
        brandSub = QLabel("PATCHER  /  LUMA IPS")
        brandSub.setObjectName("BrandSub")
        sideLayout.addWidget(brandSub)
        sideLayout.addSpacing(28)
        self.setupNav = QPushButton("First-time setup")
        self.patchNav = QPushButton("Make a Luma patch")
        for button in (self.setupNav, self.patchNav):
            button.setProperty("role", "nav")
            sideLayout.addWidget(button)

        self.setupNav.clicked.connect(lambda: self.showPage(0))
        self.patchNav.clicked.connect(lambda: self.showPage(1))
        sideLayout.addStretch(1)
        footerHint = QLabel("Keep your bootstrap titles\ninstalled after setup.")
        footerHint.setObjectName("FooterHint")
        sideLayout.addWidget(footerHint)
        shell.addWidget(sidebar)

        content = QWidget()
        contentLayout = QVBoxLayout(content)
        contentLayout.setContentsMargins(0, 0, 0, 0)
        contentLayout.setSpacing(0)
        self.pages = QStackedWidget()
        self.setupPage = CFormPage(True)
        self.patchPage = CFormPage(False)
        self.pages.addWidget(self.setupPage)
        self.pages.addWidget(self.patchPage)
        self.setupPage.runButton.clicked.connect(lambda: self.runPage(self.setupPage))
        self.patchPage.runButton.clicked.connect(lambda: self.runPage(self.patchPage))
        contentLayout.addWidget(self.pages, 1)

        footer = QFrame()
        footer.setProperty("role", "card")
        footerLayout = QVBoxLayout(footer)
        footerLayout.setContentsMargins(18, 12, 18, 14)
        footerLayout.setSpacing(8)
        statusRow = QHBoxLayout()
        self.statusLabel = QLabel("Ready")
        statusRow.addWidget(self.statusLabel)
        statusRow.addStretch(1)
        self.openButton = QPushButton("Open output folder")
        self.openButton.setProperty("role", "ghost")
        self.openButton.setEnabled(False)
        self.openButton.clicked.connect(self.openOutput)
        statusRow.addWidget(self.openButton)
        footerLayout.addLayout(statusRow)
        self.progress = QProgressBar()
        self.progress.setRange(0, 1)
        self.progress.setValue(0)
        self.progress.setTextVisible(False)
        self.progress.hide()
        footerLayout.addWidget(self.progress)
        self.log = QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumHeight(138)
        self.log.document().setMaximumBlockCount(2500)
        self.log.hide()
        footerLayout.addWidget(self.log)
        contentLayout.addWidget(footer)
        shell.addWidget(content, 1)

        self.process = QProcess(self)
        self.process.setProcessChannelMode(QProcess.MergedChannels)
        self.process.readyReadStandardOutput.connect(self.readOutput)
        self.process.finished.connect(self.taskFinished)
        self.process.errorOccurred.connect(self.processError)
        self.showPage(0)

    def showPage(self, index: int) -> None:
        self.pages.setCurrentIndex(index)
        for button, active in ((self.setupNav, index == 0), (self.patchNav, index == 1)):
            button.setProperty("active", "true" if active else "false")
            button.style().unpolish(button)
            button.style().polish(button)

    def setBusy(self, busy: bool) -> None:
        self.busy = busy
        self.setupPage.setEnabled(not busy)
        self.patchPage.setEnabled(not busy)
        self.openButton.setEnabled(not busy and self.outputPath.is_dir() and not self.checkOnly)
        self.progress.setRange(0, 0 if busy else 1)

    def setStatus(self, message: str, color: str = TEXT_DIM) -> None:
        self.statusLabel.setText(message)
        self.statusLabel.setStyleSheet(f"color: {color};")

    def appendLog(self, message: str) -> None:
        cursor = self.log.textCursor()
        cursor.movePosition(QTextCursor.End)
        cursor.insertText(message)
        self.log.setTextCursor(cursor)

    def runPage(self, page: CFormPage) -> None:
        try:
            tasks, outputPath, checkOnly = page.buildTasks()
            patcherPath = FindPatcher(page.bootstrapMode, checkOnly)
            if not checkOnly:
                (outputPath if page.bootstrapMode else outputPath.parent).mkdir(parents=True, exist_ok=True)
        except (OSError, ValueError) as error:
            QMessageBox.warning(self, "Check your inputs", str(error))
            return

        self.patcherPath = patcherPath
        self.tasks = tasks
        self.outputPath = outputPath
        self.checkOnly = checkOnly
        self.bootstrapRun = page.bootstrapMode
        self.hasUpdate = page.bootstrapMode and bool(page.updateField.path())
        self.log.clear()
        self.progress.setValue(0)
        self.progress.show()
        self.log.show()
        self.setBusy(True)
        self.startNextTask()

    def startNextTask(self) -> None:
        if not self.tasks:
            self.setBusy(False)
            self.progress.setValue(1)
            self.setStatus("Checks passed" if self.checkOnly else "Done. Your files are ready.", OK)
            if not self.checkOnly:
                if self.bootstrapRun:
                    folder = "bootstrap-update" if self.hasUpdate else "bootstrap-base"
                    self.appendLog("\nNext: install bootstrap-base/bootstrap.cia with FBI")
                    if self.hasUpdate:
                        self.appendLog(", then bootstrap-update/bootstrap.cia")

                    self.appendLog(f". Keep them installed. Copy {folder}/code.ips to /luma/titles/000400000017CA00/.\n")
                else:
                    self.appendLog("\nNext: copy code.ips to /luma/titles/000400000017CA00/ and relaunch Minecraft.\n")

            return

        name, arguments = self.tasks.pop(0)
        self.setStatus(f"Working on {name}...")
        self.appendLog(f"\n{name.upper()}\n")
        self.process.start(str(self.patcherPath), arguments)

    def readOutput(self) -> None:
        output = bytes(self.process.readAllStandardOutput()).decode("utf-8", errors="replace")
        self.appendLog(output)

    def taskFinished(self, exitCode: int, exitStatus: QProcess.ExitStatus) -> None:
        self.readOutput()
        if not self.busy:
            return

        if exitStatus != QProcess.NormalExit or exitCode != 0:
            self.tasks.clear()
            self.setBusy(False)
            self.progress.setValue(0)
            self.setStatus("The patcher stopped. See the log for details.", DANGER)
            return

        self.startNextTask()

    def processError(self, error: QProcess.ProcessError) -> None:
        if error == QProcess.FailedToStart and self.busy:
            self.tasks.clear()
            self.setBusy(False)
            self.progress.setValue(0)
            self.setStatus("Could not start the patcher executable.", DANGER)
            self.appendLog(self.process.errorString() + "\n")

    def openOutput(self) -> None:
        if self.outputPath.is_dir():
            QDesktopServices.openUrl(QUrl.fromLocalFile(str(self.outputPath)))

    def closeEvent(self, event) -> None:
        if self.busy:
            self.setStatus("Please wait for the current patch to finish.")
            event.ignore()
            return

        event.accept()


def Main() -> int:
    application = QApplication(sys.argv)
    application.setApplicationName("Minecraft Old 3DS Patcher")
    LoadFonts()
    application.setStyleSheet(STYLE)
    window = CMainWindow()
    window.show()
    return application.exec()


if __name__ == "__main__":
    raise SystemExit(Main())
