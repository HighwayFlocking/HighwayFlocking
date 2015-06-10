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

import os
import sys
import socket
import struct
import msgpack
import subprocess
import time
import logging
import threading
from io import BytesIO
from collections import defaultdict
try:
    from queue import Queue, Empty
except ImportError:
    from Queue import Queue, Empty

import config

TCP_IP = "127.0.0.1"
TCP_PORT = 5062

PT_SetPaused = 0
PT_GetBehaviors = 1
PT_DisableAllSpawners = 2
PT_ConfigureSpawner = 3
PT_ExecuteCommand = 4
PT_GetStats = 5
PT_RemoveAllVehicles = 6
PT_ResetStats = 7
PT_IncidentLog = 8
PT_RecreateLogEntry = 9
PT_AddArrow = 10
PT_StartRecording = 11
PT_StopRecording = 12
PT_StartPlayback = 13
PT_StopPlayback = 14
PT_GetGitHash = 15
PT_StartMatinee = 16
PT_SetArrowsVisible = 17
PT_SeekAndPauseReplay = 18
PT_SetReplaySpeed = 19
PT_SetReplayPaused = 20
PT_SelectCamera = 21
PT_CameraLookAtVehicle = 22
PT_SelectVehicleCamera = 23
PT_FollowCam = 24


logger = logging.getLogger(__name__)

class SimulatorIsClosedException(Exception):
    pass

class CouldNotConnectToSimulatorException(Exception):
    pass

class Simulator(object):
    def __init__(self, start=True, autoquit=True, fixed_time_step=True, dumpmovie=False,
            moviename="CommandLine", use_engine=False):
        vehicle_program = config.SIMULATOR_LOCATION
        self.command_line = [vehicle_program, '-NoTextureStreaming']
        if fixed_time_step:
            self.command_line += ['-useFixedTimeStep', '-FPS=30']
        if dumpmovie:
            # According to the unreal documentation, we could have used DUMPMOVIE, however that
            # dumps the movie to bmp, and the quality looks less than good. This is what is run
            # when we choose to record a matinee, and we use this insted.
            #
            # However, MATINEEAVICAPTURE and BENCHMARK (and probably more) only works when
            # running the editor with the game options, but not by running the binary. This means
            # that we need to do it a bit different. So this will only work if the editor is
            # installed, and in the correct location. (Change this in config)
            #
            # This also takes a looooot longer than starting the usual way.
            self.command_line = [
                config.ENGINE_LOCATION,
                config.UPROJECT_LOCATION,
                '-game',
                '-NoSplash',
                '-nomovie',
                '-ResX=1920'
                '-ResY=1080',
                '-NoTextureStreaming',
                '-BENCHMARK',
                '-FPS=60',
                '-MATINEEAVICAPTURE=%s' % moviename, # This is only used in the filename
                '-MATINEEPACKAGE=drammensveien',
                '-Multiprocess',
                '-rocket',]
        if use_engine:
            self.command_line = [
                config.ENGINE_LOCATION,
                config.UPROJECT_LOCATION,
                '-game',
                '-ResX=1920'
                '-ResY=1080',

                ]
            if fixed_time_step:
                self.command_line += [
                    '-BENCHMARK',
                    '-FPS=30',]
        self.clear_queue()
        self.start = start
        self.autoquit = autoquit
        self.closed = False
        self.thread_shutdown = False

    def __enter__(self):
        self.start_and_connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

    def clear_queue(self):
        self.packet_queue = defaultdict(Queue)

    def start_and_connect(self):
        # Start the simulator
        if self.start:
            logger.info("Starting Highway Flocking simulator: %s" % self.command_line)
            self.command = subprocess.Popen(self.command_line, stdout=subprocess.PIPE)
        else:
            self.command = None

        # Wait for it to start up, and connect to the socket
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.settimeout(1.5)
        ok = False
        for i in range(100):
            try:
                logger.info("Connecting to socket")
                self.socket.connect((TCP_IP, TCP_PORT))
            except socket.error:
                logger.info("Error connecting to socket, trying again")
                continue
            ok = True
            break

        if not ok:
            if self.command:
                self.command.kill()
            raise CouldNotConnectToSimulatorException

        self.thread = threading.Thread(target=self.receive_thread, name="Receive Thread")
        self.thread.daemon = True
        self.thread.start()

        logger.info("Successfully connected to socket")

    def close(self):
        self.thread_shutdown = True
        self.thread.join(10.0)
        if self.thread.isAlive():
            logger.warning("Thread did not shut down")

        logger.info("Closing the socket and the simulator")
        if not self.start or not self.autoquit:
            # If we did not start it, we wont close it
            self.socket.close()
            return
        # First we try to close it the nice way
        try:
            self.execute_command('quit')
        except socket.error:
            logger.exception("Error sending quit command")
            pass
        self.socket.close()
        # Wait 3 seconds, check every .5 seconds
        for i in range(6):
            time.sleep(0.5)
            if self.command.poll() != None:
                return

        # The simulator did not quit, lets kill it
        logger.warning("The simulator did not exit, killing it.")
        self.command.kill()

    def reset(self):
        self.close()
        self.closed = False
        self.clear_queue()
        self.start_and_connect()

    def send_message(self, message_type, data=None):
        msg = {
                'type': message_type,
                'data': data,
            }
        logger.debug("Sending message: %s", msg)
        message = msgpack.packb(msg)

        cnt = self.socket.send(message)
        logger.debug("Sent %d bytes", cnt)

    def receive_thread(self):
        logger.info("Receive thread started")
        unpacker = msgpack.Unpacker(encoding='utf-8')
        timeouts = 0
        try:
            while True:
                try:
                    if self.thread_shutdown:
                        logger.debug("Got thread shutdown signal, shutting down")
                        break
                    buf = self.socket.recv(1024 * 2)
                    timeouts = 0
                    logger.debug("Received: %r", buf)
                    if not buf:
                        logger.info("No data from recv, shutting down")
                        break
                    unpacker.feed(buf)
                    for o in unpacker:
                        self.process(o)
                except socket.timeout:
                    # Just retry. We want to timeout now and then to check if we should
                    # restart
                    timeouts += 1
                    if timeouts > 60:
                        # If we have waited more than one minute without any packet, we assume
                        # the simulator have stopped
                        break
                except socket.error:
                    logger.exception("Got socket error")
                    break

        finally:
            # When self.closed is True, _get_packet will raise SimulatorIsClosedException
            self.closed = True
            logger.debug("Thread shut down")

    def process(self, unpacked):
        logger.debug("Received Packet: %r", unpacked)
        packet_type = unpacked[0]
        packet = unpacked[1]
        self.packet_queue[packet_type].put(packet)

    def _get_newest_packet(self, packet_type):
        last_packet = None
        while True:
            packet = self._get_packet(packet_type, block=False)
            if not packet:
                return last_packet
            last_packet = packet

    def _get_packet(self, packet_type, block=True):
        logger.debug("Waiting for packet of type %d" % packet_type)
        while True:
            try:
                packet = self.packet_queue[packet_type].get(block=block, timeout=1)
                logger.debug("Got a packet: %s" % packet)
                return packet
            except Empty:
                logger.debug("Got no packets!")
                if self.closed:
                    raise SimulatorIsClosedException
                if not block:
                    return None
                # If we are blocking, just continue the loop

    def set_paused(self, set_paused):
        self.send_message(PT_SetPaused, set_paused)

    def get_behaviors(self):
        self.send_message(PT_GetBehaviors)
        data = self._get_packet(PT_GetBehaviors)
        return data

    def configure_spawner(self, config):
        self.send_message(PT_ConfigureSpawner, config)

    def reset_all_spawners(self):
        self.send_message(PT_DisableAllSpawners)

    def receive_stats(self):
        data = self._get_packet(PT_GetStats)
        return data

    def get_newest_stats(self):
        data = self._get_newest_packet(PT_GetStats)
        return data

    def get_log_entries(self):
        while True:
            data = self._get_packet(PT_IncidentLog, block=False)
            if not data:
                break
            yield data

    def execute_command(self, command):
        self.send_message(PT_ExecuteCommand, command)

    def remove_all_vehicles(self):
        self.send_message(PT_RemoveAllVehicles)

    def reset_stats(self):
        self.send_message(PT_ResetStats)

    def recreate_log_entry(self, vehicles):
        self.send_message(PT_RecreateLogEntry, vehicles)

    def add_arrow(self, location, rotation):
        self.send_message(PT_AddArrow, [location, rotation])

    def start_recording(self, filename):
        filename = os.path.abspath(filename)
        self.send_message(PT_StartRecording, filename)

    def stop_recording(self):
        self.send_message(PT_StopRecording)

    def start_playback(self, filename):
        filename = os.path.abspath(filename)
        self.send_message(PT_StartPlayback, filename)

    def stop_playback(self):
        self.send_message(PT_StopPlayback)

    def get_git_hash(self):
        self.send_message(PT_GetGitHash)
        return self._get_packet(PT_GetGitHash)

    def start_matinee(self, name):
        self.send_message(PT_StartMatinee, name)

    def set_arrows_visible(self, visible, behaviors):
        self.send_message(PT_SetArrowsVisible, {
            'visible': visible,
            'behaviors': behaviors})

    def seek_and_pause_replay(self, seconds):
        seconds = float(seconds)
        self.send_message(PT_SeekAndPauseReplay, seconds)
        return self._get_packet(PT_SeekAndPauseReplay)

    def set_replay_speed(self, speed):
        self.send_message(PT_SetReplaySpeed, speed)

    def set_replay_paused(self, paused):
        self.send_message(PT_SetReplayPaused, paused)

    def select_camera(self, cameraname):
        self.send_message(PT_SelectCamera, cameraname)

    def camera_look_at_vehicle(self, vehicleid):
        self.send_message(PT_CameraLookAtVehicle, vehicleid)

    def select_vehicle_camera(self, vehicleid, cameraname):
        self.send_message(PT_SelectVehicleCamera,
            {'vehicle_id': vehicleid, 'camera_name': cameraname})

    def followcam(self, speed, offset_z, offset_y, look_distance, rev, look_offset_y, look_offset_z,
                  fov, start_distance, fsmooth):
        self.send_message(PT_FollowCam,
            {
                'speed': speed,
                'offset_z': offset_z,
                'offset_y': offset_y,
                'look_offset_z': look_offset_z,
                'look_offset_y': look_offset_y,
                'look_distance': look_distance,
                'reversed': rev,
                'field_of_view': fov,
                'start_distance': start_distance,
                'follow_smooth': fsmooth,
            })
