#coding: utf-8

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

import sys
import logging
import os
import threading
from datetime import time
import subprocess

from PySide import QtCore, QtGui
from PySide.QtCore import QTimer

from lib.gui.controller_ui import Ui_MainWindow
import configs
from lib.simulation import Simulator, SimulatorIsClosedException

logger = logging.getLogger(__name__)

configurations = (
        ('just_cars', configs.JUST_CARS),
        ('oncoming', configs.ONCOMING),
        ('oncoming + merging', configs.ONCOMING_ONRAMP),
        ('oncoming + merging + buses', configs.ONCOMING_ONRAMP_BUS),
        ('oncoming + merging + emergency vehicles', configs.ONCOMING_ONRAMP_EMERGENCY),
        ('oncoming + merging + buses + emergency vehicles', configs.ONCOMING_ONRAMP_BUS_EMERGENCY),
        ('symetric', configs.SYMETRIC)
    )

class ControllerMainWindow(QtGui.QMainWindow):
    def __init__(self, parent=None):
        super(ControllerMainWindow, self).__init__(parent)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.setup_configurations()
        self.assign_widgets()

        self.simulator = None
        self.ui.stop.setEnabled(False)

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update)
        self.timer.start(1000)

        self.show()

        desktop = QtGui.QDesktopWidget().availableGeometry()

        y = self.y()
        x = (desktop.width() - 1280) / 2 - self.width() - 50

        self.move(x, y)

    def setup_configurations(self):
        for i, config in enumerate(configurations):
            self.ui.configuration.insertItem(i, config[0])

    def assign_widgets(self):
        self.ui.start.clicked.connect(self.start_clicked)
        self.ui.stop.clicked.connect(self.stop_clicked)

    def on_about_to_quit(self):
        if self.simulator:
            self.simulator.close()

    def stop_clicked(self):
        self.ui.stop.setEnabled(False)
        if self.simulator:
            self.simulator.close()
            self.ui.fixed.setEnabled(True)
            self.simulator = None
            self.ui.start.setText("Start Simulation")

    def start_clicked(self):
        self.ui.start.setEnabled(False)
        self.ui.fixed.setEnabled(False)
        if self.simulator:
            self.update_simulator()
        else:
            self.simulator = Simulator(fixed_time_step=self.ui.fixed.isChecked())
            threading.Thread(target=self.start_simulator, name="Start simulator Thread").start()

            self.ui.status.setText(
                    "<html><head/><body><p><span style=\"color:#222;\">Simulator is Starting</span></p></body></html>")

            self.ui.start.setText("Restart Simulation")

    def start_simulator(self):
        self.simulator.start_and_connect()
        self.update_simulator()


    def update_simulator(self):
        base_config = configurations[self.ui.configuration.currentIndex()][1]
        throughput = self.ui.throughput.value()
        config = configs.througput(base_config, throughput=throughput)

        logger.info("Max Waits: %s", [sp['max_wait'] for sp in config['spawners']])
        logger.info("Min Waits: %s", [sp['min_wait'] for sp in config['spawners']])

        logger.info('Pausing the simulation')
        self.simulator.set_paused(True)
        logger.info('Removing all vehicles')
        self.simulator.remove_all_vehicles()
        logger.info('Resetting the spawners')
        self.simulator.reset_all_spawners()

        logger.info('Configuring the spawners')
        for spawner_conf in config['spawners']:
            self.simulator.configure_spawner(spawner_conf)

        logger.info('Starting the simulation')

        self.simulator.set_paused(False)

        logger.info('Resetting the stats')
        self.simulator.reset_stats()

        self.simulator.clear_queue()
        self.ui.start.setEnabled(True)
        self.ui.stop.setEnabled(True)

    def update(self):
        if self.simulator:
            try:
                stats = self.simulator.get_newest_stats()
                if stats:
                    minutes, seconds = divmod(int(stats['time']), 60)
                    stats['time'] = time(minute=minutes, second=seconds)
                    self.ui.current_values.setText(
"""Time: {time:%M:%S}
Current Throughput: From City: {throughputs[0]}, To City: {throughputs[1]}
Incidents: {incidents}
Vehicles Spawned: {spawned}
Vehicles on Road: {onroad}""".format(**stats))
                    self.ui.status.setText(
                        "<html><head/><body><p><span style=\"color:#00b548;\">Simulator is Running</span></p></body></html>")
            except SimulatorIsClosedException:
                self.ui.status.setText(
                    "<html><head/><body><p><span style=\"color:#b50003;\">Simulator is not Running</span></p></body></html>")
                self.simulator.close()
                self.simulator = None
                self.ui.start.setText("Start Simulation")
                self.ui.stop.setEnabled(False)
                self.ui.fixed.setEnabled(True)
                return

def main():
    fh = logging.FileHandler('gui.log')
    fh.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(funcName)s - %(message)s')
    fh.setFormatter(formatter)
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)

    app = QtGui.QApplication(sys.argv)
    controllerWindow = ControllerMainWindow()
    app.aboutToQuit.connect(controllerWindow.on_about_to_quit)
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()
