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

"""Run Trials.

Usage:
  worker.py [SERVER_URL]
  worker.py (-h | --help)
  worker.py --version

Options:
  -h --help           Show this screen.
  --version           Show version.

"""

# TODO: Do something intelligent if the server does not answer

import logging
import os
import time
import socket
import os.path
import sys

import requests
from docopt import docopt

from lib.simulation import Simulator
from lib import network
import config

__version__ = "0.2.1"

# Changelog

# 0.1.2
# ThrottleOutput should not anymore be NaN

# 0.1.5
# Terminating Vehicle.exe before upgrading

# 0.1.6
# Saving version number to config
# Specific front distance for busses

# 0.1.7
# Vehicle builds in the git hash
# Front distance for busses also work on cars behind
# Save git hash to config

# 0.1.8
# Higher comfort sones for avoid

# 0.1.9
# Use road tangent for forward vector in avoid

# 0.2.0
# Do not log crash to postgresql
# Do not remove crashed vehicles

# 0.2.1
# Save wheel movement

CURSOR_UP_ONE = '\x1b[1A'
ERASE_LINE = '\x1b[2K'

PATH = os.path.dirname(os.path.realpath(__file__))

logger = logging.getLogger(__name__)

class Worker(object):
    def __init__(self):
        self.do_run = True
        self.name = None
        self.taskid = None

    def run_task(self, config, simulator):
        logger.info('Pausing the simulation')
        simulator.set_paused(True)
        logger.info('Removing all vehicles')
        simulator.remove_all_vehicles()
        logger.info('Resetting the spawners')
        simulator.reset_all_spawners()

        logger.info('Configuring the spawners')
        for spawner_conf in config['spawners']:
            simulator.configure_spawner(spawner_conf)

        logger.info("Starting the recording")

        try:
            os.mkdir('recordings')
        except WindowsError:
            pass
        replayname = time.strftime('recordings/recording_%Y.%m.%d_%H.%M.%S.bin')
        simulator.start_recording(replayname)

        logger.info('Starting the simulation')

        simulator.set_paused(False)

        logger.info('Resetting the stats')
        simulator.reset_stats()

        logger.info('Warming up the simulation')

        time_start = time.time()

        simulator.clear_queue()

        print ''

        while True:
            if not self.do_run:
                raise KeyboardInterrupt

            stats = simulator.receive_stats()
            logger.debug("(Warmup) Got stats: %s", stats)
            if time.time() - time_start < 2.0:
                # Ensure we wait at least 2 seconds, in case of old data
                continue
            if stats['time'] >= config['warmup_time']:
                break

            print(CURSOR_UP_ONE + ERASE_LINE + CURSOR_UP_ONE)
            print '  '.join(['%s: %s' % (key, value) for key, value in stats.items()])

        logger.info("Warmup complete!")

        timeline = []
        log = []

        print ''

        max_time = config['run_time'] + config['warmup_time']

        last_status = -100.0

        while True:
            if not self.do_run:
                raise KeyboardInterrupt

            stats = simulator.receive_stats()

            timeline.append(stats)
            logger.debug("Got stats: %s", stats)
            print(CURSOR_UP_ONE + ERASE_LINE + CURSOR_UP_ONE)
            print '  '.join(['%s: %s' % (key, value) for key, value in stats.items()])

            for entry in simulator.get_log_entries():
                log.append(entry)
                logger.info("Got log entry")
                logger.debug("Got log entry: %s", entry)

            current_time = stats['time']

            if current_time >= max_time:
                break

            percent_complete = (current_time / max_time) * 100.0
            if current_time - last_status > 60.0:
                try:
                    network.post_status(
                        workername=self.name,
                        taskid=self.taskid,
                        percent_completed=percent_complete)
                except requests.ConnectionError, requests.HTTPError:
                    logger.warning("Could not send status to server")
                last_status = current_time

        logger.info("Stopping Recording")

        simulator.stop_recording()

        logger.info("Completed run")

        incidents = timeline[-1]['incidents'] - timeline[0]['incidents']
        throughputs = [d['throughputs'][1] for d in timeline]
        avg_throughput = sum(throughputs) / len(throughputs)

        results = {
            'timeline': timeline,
            'log': log,
            'incidents': incidents,
            'avg_throughput': avg_throughput,
            }

        return results, replayname


    def run_worker(self):
        logger.info("Starting Worker")
        self.name = socket.gethostname()
        logger.info("Hello! My name is %s", self.name)
        while self.do_run:
            #Ensure that the HighwayFlocking.exe from last time is killed
            os.system('taskkill /F /IM HighwayFlocking.exe /T')
            logger.info("Getting the task from the server")
            try:
                task = network.get_task_for_completion(self.name, __version__)
            except requests.ConnectionError, requests.HTTPError:
                logger.info("Server is not up, sleeping and trying again")
                time.sleep(10)
                continue
            if not task:
                logger.info("No new task from the server, sleeping and trying again")
                time.sleep(10)
                continue
            if task['type'] == 'simulator':
                config = task['config']
                self.taskid = task['id']

                with Simulator() as simulator:
                    simulator.execute_command('r.setres 200x100')
                    config['behaviours'] = simulator.get_behaviors()
                    config['command_line'] = simulator.command_line
                    config['version'] = __version__
                    logger.info("Worker version: %s", config['version'])
                    config['simulator_version'] = simulator.get_git_hash()
                    logger.info("Simuator version: %s", config['simulator_version'])
                    logger.debug("Using config: %s", config)
                    logger.info("Max Waits: %s", [sp['max_wait'] for sp in config['spawners']])
                    logger.info("Min Waits: %s", [sp['min_wait'] for sp in config['spawners']])

                    logger.info("Running the task")
                    results, replayname = self.run_task(config, simulator)
                logger.info("Task completed, sending the result to the server")
                network.post_result(self.name, task['id'], config, results)
                network.post_replay(task['id'], replayname, self.name)
            elif task['type'] == 'upgrade':
                filename = network.download_worker()
                # Be really sure that Vehicle is not running
                os.system('taskkill /F /IM HighwayFlocking.exe /T')
                logger.info("Unpacking worker")
                os.system('unzip -o %s' % filename)
                logger.info("Running the new worker")
                args = sys.argv[:]

                args.insert(0, sys.executable)

                logger.info('Re-spawning %s' % ' '.join(args))

                if sys.platform == 'win32':
                    args = ['"%s"' % arg for arg in args]

                os.execv(sys.executable, args)




    def main(self):
        self.setup_logging()
        arguments = docopt(__doc__, version=__version__)
        if arguments['SERVER_URL']:
            config.SERVER_URL = arguments['SERVER_URL']

        while self.do_run:
            try:
                self.run_worker()
            except KeyboardInterrupt:
                break
            except Exception:
                time.sleep(30)
                logger.exception("Got an exception")


    def setup_logging(self):
        try:
            os.mkdir('log')
        except WindowsError:
            pass
        logname = time.strftime(os.path.join(PATH, 'log/worker_%Y.%m.%d_%H.%M.log'))
        fh = logging.FileHandler(logname)
        fh.setLevel(logging.DEBUG)
        ch = logging.StreamHandler()
        ch.setLevel(logging.INFO)
        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(funcName)s - %(message)s')
        fh.setFormatter(formatter)
        ch.setFormatter(formatter)
        global_logger = logging.getLogger('')
        global_logger.setLevel(logging.DEBUG)
        global_logger.addHandler(fh)
        global_logger.addHandler(ch)

if __name__ == '__main__':
    Worker().main()
