# -*- coding: utf-8 -*-

# Copyright 2015 Sindre Ilebekk Johansen and Andreas Sløgedal Løvland

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Form implementation generated from reading ui file 'controller.ui'
#
# Created: Sat May 30 19:17:14 2015
#      by: pyside-uic 0.2.15 running on PySide 1.2.2
#
# WARNING! All changes made in this file will be lost!

from PySide import QtCore, QtGui

class Ui_MainWindow(object):
    def setupUi(self, MainWindow):
        MainWindow.setObjectName("MainWindow")
        MainWindow.resize(358, 205)
        self.centralwidget = QtGui.QWidget(MainWindow)
        self.centralwidget.setObjectName("centralwidget")
        self.gridLayout = QtGui.QGridLayout(self.centralwidget)
        self.gridLayout.setObjectName("gridLayout")
        self.configuration = QtGui.QComboBox(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(10)
        self.configuration.setFont(font)
        self.configuration.setObjectName("configuration")
        self.gridLayout.addWidget(self.configuration, 0, 1, 1, 1)
        self.label_2 = QtGui.QLabel(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Maximum, QtGui.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.label_2.sizePolicy().hasHeightForWidth())
        self.label_2.setSizePolicy(sizePolicy)
        self.label_2.setObjectName("label_2")
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.frame = QtGui.QFrame(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Maximum)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.frame.sizePolicy().hasHeightForWidth())
        self.frame.setSizePolicy(sizePolicy)
        self.frame.setFrameShape(QtGui.QFrame.StyledPanel)
        self.frame.setFrameShadow(QtGui.QFrame.Raised)
        self.frame.setObjectName("frame")
        self.horizontalLayout = QtGui.QHBoxLayout(self.frame)
        self.horizontalLayout.setObjectName("horizontalLayout")
        self.fixed = QtGui.QCheckBox(self.frame)
        self.fixed.setChecked(True)
        self.fixed.setTristate(False)
        self.fixed.setObjectName("fixed")
        self.horizontalLayout.addWidget(self.fixed)
        self.gridLayout.addWidget(self.frame, 2, 0, 1, 2)
        self.status = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(10)
        self.status.setFont(font)
        self.status.setObjectName("status")
        self.gridLayout.addWidget(self.status, 6, 0, 1, 2)
        self.throughput = QtGui.QSpinBox(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Minimum, QtGui.QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.throughput.sizePolicy().hasHeightForWidth())
        self.throughput.setSizePolicy(sizePolicy)
        font = QtGui.QFont()
        font.setPointSize(10)
        self.throughput.setFont(font)
        self.throughput.setButtonSymbols(QtGui.QAbstractSpinBox.PlusMinus)
        self.throughput.setMinimum(2000)
        self.throughput.setMaximum(15000)
        self.throughput.setSingleStep(1000)
        self.throughput.setProperty("value", 4000)
        self.throughput.setObjectName("throughput")
        self.gridLayout.addWidget(self.throughput, 1, 1, 1, 1)
        self.label = QtGui.QLabel(self.centralwidget)
        sizePolicy = QtGui.QSizePolicy(QtGui.QSizePolicy.Maximum, QtGui.QSizePolicy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.label.sizePolicy().hasHeightForWidth())
        self.label.setSizePolicy(sizePolicy)
        self.label.setTextFormat(QtCore.Qt.AutoText)
        self.label.setScaledContents(False)
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.current_values = QtGui.QLabel(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(10)
        self.current_values.setFont(font)
        self.current_values.setText("")
        self.current_values.setObjectName("current_values")
        self.gridLayout.addWidget(self.current_values, 8, 0, 1, 2)
        self.gridLayout_2 = QtGui.QGridLayout()
        self.gridLayout_2.setObjectName("gridLayout_2")
        self.start = QtGui.QPushButton(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(10)
        font.setWeight(50)
        font.setBold(False)
        self.start.setFont(font)
        self.start.setObjectName("start")
        self.gridLayout_2.addWidget(self.start, 0, 0, 1, 1)
        self.stop = QtGui.QPushButton(self.centralwidget)
        font = QtGui.QFont()
        font.setPointSize(10)
        self.stop.setFont(font)
        self.stop.setObjectName("stop")
        self.gridLayout_2.addWidget(self.stop, 0, 1, 1, 1)
        self.gridLayout.addLayout(self.gridLayout_2, 5, 0, 1, 2)
        MainWindow.setCentralWidget(self.centralwidget)
        self.statusbar = QtGui.QStatusBar(MainWindow)
        self.statusbar.setObjectName("statusbar")
        MainWindow.setStatusBar(self.statusbar)

        self.retranslateUi(MainWindow)
        QtCore.QMetaObject.connectSlotsByName(MainWindow)

    def retranslateUi(self, MainWindow):
        MainWindow.setWindowTitle(QtGui.QApplication.translate("MainWindow", "Highway Simulator Controller", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("MainWindow", "<html><head/><body><p><span style=\" font-size:10pt;\">Throughput:</span></p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.fixed.setText(QtGui.QApplication.translate("MainWindow", "Fixed Time Step", None, QtGui.QApplication.UnicodeUTF8))
        self.status.setText(QtGui.QApplication.translate("MainWindow", "<html><head/><body><p><span style=\" color:#b50003;\">Simulator is not Running</span></p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("MainWindow", "<html><head/><body><p><span style=\" font-size:10pt;\">Configuration:</span></p></body></html>", None, QtGui.QApplication.UnicodeUTF8))
        self.start.setText(QtGui.QApplication.translate("MainWindow", "Start Simulation", None, QtGui.QApplication.UnicodeUTF8))
        self.stop.setText(QtGui.QApplication.translate("MainWindow", "Stop Simulation", None, QtGui.QApplication.UnicodeUTF8))

