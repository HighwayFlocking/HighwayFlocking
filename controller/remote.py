#! python
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

"""Distributed Remote.

Usage:
  remote.py [--server SERVER_URL] add_task CONFIG --throughput VALUE [--short]
  remote.py [--server SERVER_URL] add_tasks CONFIG START END STEP [-n REPEATIONS]
  remote.py [--server SERVER_URL] tasks [--all]
  remote.py [--server SERVER_URL] results [CONFIG] [--with_version VERSION]
  remote.py [--server SERVER_URL] result RESULTID
  remote.py [--server SERVER_URL] workers
  remote.py [--server SERVER_URL] cleanup
  remote.py [--server SERVER_URL] plot CONFIG (tp|name|wtp) [--with_version VERSION --pdf --stdout -r --max-tp VALUE]
  remote.py [--server SERVER_URL] play_replay [--existing] RESULTID [START_POS]
  remote.py play_file [--existing] FILENAME
  remote.py [--server SERVER_URL] reset_task TASKID
  remote.py movie [--existing] RESULTID START_POS END_POS [--show-arrows --beh BEHAVIORS]
  remote.py movie [--existing] RESULTID START_POS END_POS [--show-arrows --beh BEHAVIORS] mat MATINEE_NAME [--follow VEHICLE_ID] [--record]
  remote.py movie [--existing] RESULTID START_POS END_POS [--show-arrows --beh BEHAVIORS] cam CAMERA_NAME [--follow VEHICLE_ID] [--record]
  remote.py movie [--existing] RESULTID START_POS END_POS [--show-arrows --beh BEHAVIORS] followcam [reversed] --fov FOV --speed SPEED [--start-dist START_DIST] --offz OFFSET_Z --offy OFFSET_Y [--fsmooth VALUE] (--dist LOOK_DISTANCE [--loffz LOOK_OFFSET_Z --loffy LOOK_OFFSET_Y] | --follow VEHICLE_ID) [--record]
  remote.py movie [--existing] RESULTID START_POS END_POS [--show-arrows --beh BEHAVIORS] veh VEHICLE_ID CAMERA_NAME [--record]
  remote.py run_locally [--existing] CONFIG --throughput VALUE [--short]
  remote.py engine_record REPLAYNAME
  remote.py (-h | --help)
  remote.py --version

Options:
  -h --help           Show this screen.
  --version           Show version.
  --norepeat          Do not wrap everything in a while-loop
  -t --throughput VALUE  Just run with this value as throughput
  -n REPEATIONS       Repeat the tasks this number of times [default: 1]
  --server SERVER_URL  The server to use, if not using the default
  -v --with_version VERSION  Force this version
  --pdf               Export the plot as latexified pdf
  -r                  Use R to plot (requires the plot.R file)
  --max-tp VALUE      The maximal throughput for a plot
  -e --existing       Use an already existing simulator instance
  --follow VEHICLE_ID  Make the camera follow this vehicle
  --record         Dump movie to file
  --show-arrows        Show arrows for behaviors
  --beh BEHAVIORS  The behaviors to show arrow for
  --start-dist START_DIST  The starting distance for the followcam [default: 0]
  --fov FOV  The field of view
  --speed SPEED the speed
  --offz OFFSET_Z  Offset upwards
  --offy OFFSET_Y  Offset normal to the road
  --fsmooth VALUE  The ammount to smooth the follow value. [default: 0.95]

"""

import logging
import re
import os
import os.path
from datetime import datetime
from pprint import pprint
import time
from collections import Counter
import csv
import sys
import subprocess
from cStringIO import StringIO

from docopt import docopt
import matplotlib.pyplot as plt
import numpy as np
#import seaborn as sns

import configs
from lib import network
from lib.simulation import Simulator
from lib.plot_utils import latexify, format_axes
import config

logger = logging.getLogger(__name__)

CURSOR_UP_ONE = '\x1b[1A'
ERASE_LINE = '\x1b[2K'

def main():
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(funcName)s - %(message)s')
    ch.setFormatter(formatter)
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(ch)

    arguments = docopt(__doc__, version='0.1')

    if arguments['--server']:
        config.SERVER_URL = arguments['--server']

    if arguments['add_task']:
        add_task(arguments['CONFIG'], float(arguments['--throughput']), arguments['--short'])
    if arguments['run_locally']:
        run_locally(
            arguments['--existing'],
            arguments['CONFIG'],
            float(arguments['--throughput']),
            arguments['--short'])
    if arguments['add_tasks']:
        add_tasks(arguments['CONFIG'],
                 float(arguments['START']),
                 float(arguments['END']),
                 float(arguments['STEP']),
                 int(arguments['-n']))
    elif arguments['tasks']:
        get_tasks(arguments['--all'])
    elif arguments['results']:
        get_results(config_name=arguments['CONFIG'],
                    version=arguments['--with_version'])
    elif arguments['result']:
        get_result(int(arguments['RESULTID']))
    elif arguments['reset_task']:
        reset_task(int(arguments['TASKID']))
    elif arguments['play_replay']:
        play_replay(int(arguments['RESULTID']), arguments['--existing'], arguments['START_POS'])
    elif arguments['play_file']:
        play_file(arguments['FILENAME'], arguments['--existing'])
    elif arguments['cleanup']:
        cleanup()
    elif arguments['workers']:
        get_workers()
    elif arguments['movie']:
        record_movie(
            existing=arguments['--existing'],
            matinee=arguments['MATINEE_NAME'],
            camera=arguments['CAMERA_NAME'],
            vehicle=arguments['VEHICLE_ID'],
            camera_name=arguments['CAMERA_NAME'],
            follow=arguments['--follow'],
            resultid=arguments['RESULTID'],
            startpos=arguments['START_POS'],
            endpos=arguments['END_POS'],
            record=arguments['--record'],
            followcam=arguments['followcam'],
            speed=arguments['--speed'],
            offset_z=arguments['--offz'],
            offset_y=arguments['--offy'],
            start_dist=arguments['--start-dist'],
            look_distance=arguments['LOOK_DISTANCE'],
            look_offset_z=arguments['LOOK_OFFSET_Z'],
            look_offset_y=arguments['LOOK_OFFSET_Y'],
            fov=arguments['--fov'],
            rev=arguments['reversed'],
            show_arrows=arguments['--show-arrows'],
            behaviors=arguments['--beh'],
            fsmooth=arguments['--fsmooth'])
    elif arguments['plot']:
        plot(
            config_name=arguments['CONFIG'],
            version=arguments['--with_version'],
            pdf=arguments['--pdf'],
            r=arguments['-r'],
            stdout=arguments['--stdout'],
            max_tp=int(arguments['--max-tp']) if arguments['--max-tp'] else None,
            tp=arguments['tp'],
            name=arguments['name'],
            wtp=arguments['wtp'])
    elif arguments['engine_record']:
        engine_record(arguments['REPLAYNAME'])

def record_movie(existing, matinee, camera, vehicle, camera_name, follow, resultid, startpos, record,
                 followcam, speed, offset_y, offset_z, look_distance, look_offset_z, look_offset_y,
                 rev, endpos, fov, show_arrows, behaviors, start_dist, fsmooth):
    # Since - looks like a parameter, we replace m by -, this way m can be used instead of
    # - at the command line.

    offset_y = offset_y.replace('m', '-') if offset_y else None
    offset_z = offset_z.replace('m', '-') if offset_z else None
    look_distance = look_distance.replace('m', '-') if look_distance else None
    look_offset_z = look_offset_z.replace('m', '-') if look_offset_z else None
    look_offset_y = look_offset_y.replace('m', '-') if look_offset_y else None

    behaviors = behaviors.split(',') if behaviors else []
    behaviors = [a.strip() for a in behaviors]

    if not os.path.exists('local_replays'):
        os.makedirs('local_replays')

    minutes = 0
    try:
        seconds = float(startpos)
    except ValueError:
        minutes, seconds = [float(a) for a in startpos.split(':')]

    total_seconds = seconds + minutes * 60

    end_minutes = 0
    try:
        end_seconds = int(endpos)
    except ValueError:
        end_minutes, end_seconds = [int(a) for a in endpos.split(':')]

    end_total_seconds = end_seconds + end_minutes * 60

    try:
        resultid = int(resultid)
        filename = os.path.join('local_replays', 'replay_%d.bin' % resultid)
        if not os.path.exists(filename):
            network.download_replay(resultid, filename)
    except ValueError:
        filename = resultid
        resultid = os.path.split(filename)[-1]

    print filename

    moviename = "%s_%02d%02d" % (resultid, minutes, seconds)
    if matinee:
        moviename += "_mat_%s" % matinee
    if camera:
        moviename += "_cam_%s" % camera
    if vehicle:
        moviename += "_veh_%s_%s" % (vehicle, camera_name)
    if followcam:
        moviename += "_followcam_%s_%s_%s_%s" % (
            speed, offset_z, offset_y, fov)
        if look_distance:
            moviename += "_%s_%s_%s" % (look_distance, look_offset_z, look_offset_y)
        if rev:
            moviename += "_rev"
    if follow:
        moviename += "_follow_%s" % follow
    with Simulator(
            start=not existing,
            autoquit=True,
            fixed_time_step=True,
            dumpmovie=record,
            moviename=moviename) as simulator:
        if not record:
            simulator.execute_command('r.setres 1920x1080')
        # First we remove all existing vehicles
        simulator.set_paused(True)
        simulator.remove_all_vehicles()

        simulator.set_arrows_visible(show_arrows, behaviors)

        simulator.start_playback(filename)
        if total_seconds > 0:
            simulator.seek_and_pause_replay(total_seconds)
        simulator.set_replay_paused(False)
        if matinee:
            simulator.start_matinee('Matinee' + matinee)
        if camera:
            simulator.select_camera('Camera' + camera)
        if vehicle:
            simulator.select_vehicle_camera(int(vehicle), camera_name)
        if follow:
            simulator.camera_look_at_vehicle(int(follow))
        if followcam:
            simulator.followcam(
                speed=float(speed),
                offset_z=float(offset_z),
                offset_y=float(offset_y),
                look_distance=float(look_distance) if look_distance else 0.0,
                rev=rev,
                look_offset_y=float(look_offset_y) if look_offset_y else 0.0,
                look_offset_z=float(look_offset_z) if look_offset_z else 0.0,
                fov=float(fov),
                start_distance=float(start_dist),
                fsmooth=float(fsmooth)
                )


        print('')
        while True:
            stats = simulator.receive_stats()
            total_seconds = stats['time']
            minutes, seconds = divmod(total_seconds, 60)
            print(CURSOR_UP_ONE + ERASE_LINE + CURSOR_UP_ONE)
            print('%02d:%02d / %02d:%02d' % (minutes, seconds, end_minutes, end_seconds))

            if total_seconds >= end_total_seconds:
                break

def engine_record(replayname):
    with Simulator(autoquit=False, fixed_time_step=True, use_engine=True) as simulator:
        simulator.start_recording(replayname)


def run_locally(existing, config_name, throughput, short):
    config_name = config_name.upper()
    base_config = getattr(configs, config_name)
    config = configs.througput(base_config, throughput=throughput)

    if short:
        config['warmup_time'] = 30
        config['run_time'] = 30

    with Simulator(start=not existing) as simulator:
        logger.info("Max Waits: %s", [sp['max_wait'] for sp in config['spawners']])
        logger.info("Min Waits: %s", [sp['min_wait'] for sp in config['spawners']])

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
            stats = simulator.receive_stats()

            timeline.append(stats)
            logger.debug("Got stats: %s", stats)


            for entry in simulator.get_log_entries():
                log.append(entry)
                logger.info("Got log entry")
                logger.debug("Got log entry: %s", entry)

            current_time = stats['time']

            if current_time >= max_time:
                break

            percent_complete = (current_time / max_time) * 100.0

            print(CURSOR_UP_ONE + ERASE_LINE + CURSOR_UP_ONE)
            print('  '.join(['%s: %s' % (key, value) for key, value in stats.items()])
                    + " %.1f %%" % percent_complete)

        logger.info("Stopping Recording")

        simulator.stop_recording()

        logger.info("Completed run")

        incidents = timeline[-1]['incidents'] - timeline[0]['incidents']
        throughputs = [d['throughputs'][1] for d in timeline]
        avg_throughput = sum(throughputs) / len(throughputs)

        logger.info("Avg Throughput: %.2f" % avg_throughput)
        logger.info("Incidents: %d" % incidents)


def add_task(config_name, throughput, short):
    config_name = config_name.upper()
    base_config = getattr(configs, config_name)
    config = configs.througput(base_config, throughput=throughput)

    if short:
        config['warmup_time'] = 30
        config['run_time'] = 30

    network.add_task(config)

def add_tasks(config_name, start, end, step, n=1):
    task_configs = []
    config_name = config_name.upper()
    base_config = getattr(configs, config_name)
    for throughput in np.arange(start, end, step):
        config = configs.througput(base_config, throughput=throughput)

        task_configs.append(config)

    task_configs = task_configs * n

    network.add_tasks(task_configs)

def get_tasks(get_all):
    tasks = network.get_tasks(get_all=get_all)
    print 'ID   REQUEST_TIME     STARTED_TIME     STATUS_TIME      %% DONE COMPLETED WORKER     CONFIG_NAME'
    print '==== ================ ================ ================ ======= ========= ========== =========================='
    for task in tasks:
        if task['started_time'] is not None:
            task['started_time'] = task['started_time'].strftime('%d.%m.%Y %H:%M')
        else:
            task['started_time'] = 'Not started'

        if task['status_time'] is not None:
            task['status_time'] = task['status_time'].strftime('%d.%m.%Y %H:%M')
        else:
            task['status_time'] = 'Not started'

        print(
            '{id:3d} {request_time:%d.%m.%Y %H:%M} {started_time:16s} {status_time:16s}'
            '{percent_done:6.1f} % {completed!r:9s} {worker:10s} {config[name]}'
            .format(**task))

def get_result(resultid):
    result = network.get_result(resultid)
    del result['results']['timeline']
    del result['results']['log']
    pprint(result)

def get_results(config_name, version):
    if config_name:
        config_name = config_name.upper()
        base_config = getattr(configs, config_name)
    else:
        base_config = {'name': ''}
    config = configs.remove_wait_times(base_config)
    if not config_name:
        config['name'] = ''
    if version:
        config['version'] = version

    results = network.get_results(config)
    print 'ID  TASKID VERSION COMPLETED_TIME   WORKER     CONFIG_NAME                                        INCIDENTS THROUGPUT'
    print '=== ====== ======= ================ ========== ================================================== ========= ========='
    for result in results:
        print(
            '{id:3d} {taskid:6d} {config[version]:7s} {completed_time:%d.%m.%Y %H:%M} '
            '{worker:10s} {config[name]:50s} {incidents:9.0f} {avg_throughput:9.0f}'
            .format(**result))

def get_workers():
    results = network.get_workers()
    print ' NAME       VERSION  TIME SINCE SEEN  DOING'
    print ' ========== ======== ===============  ==============='
    for worker in results:
        last_seen = worker['last_seen']
        rem = int(last_seen.total_seconds())
        hours, rem = divmod(rem, 3600)
        minutes, seconds = divmod(rem, 60)

        print (' {name:10s}  {version:8s}  {hours:02d}:{minutes:02d}:{seconds:02d}         {doing:s}'.format(
            hours=hours,
            minutes=minutes,
            seconds=seconds,
            **worker))


def play_replay(resultid, existing, startpos):
    seconds = None
    if startpos:
        try:
            seconds = int(startpos)
        except ValueError:
            minutes, seconds = [int(a) for a in startpos.split(':')]
            seconds += minutes * 60

    if not os.path.exists('local_replays'):
        os.makedirs('local_replays')

    filename = os.path.join('local_replays', 'replay_%d.bin' % resultid)
    if not os.path.exists(filename):
        network.download_replay(resultid, filename)

    with Simulator(start=not existing, autoquit=False, fixed_time_step=False) as simulator:
        simulator.set_paused(True)
        simulator.remove_all_vehicles()
        simulator.start_playback(filename)
        if seconds:
            simulator.seek_and_pause_replay(seconds)
        simulator.set_replay_paused(False)

def play_file(filename, existing):
    if not os.path.exists(filename):
        logger.error("File does not exists")
        return

    with Simulator(start=not existing, autoquit=False, fixed_time_step=False) as simulator:
        simulator.set_paused(True)
        simulator.remove_all_vehicles()
        simulator.start_playback(filename)

def reset_task(taskid):
    network.reset_task(taskid)

def cleanup():
    network.cleanup()

def plot(config_name, version, pdf, r, stdout, max_tp, tp, name, wtp):
    config_name = config_name.upper()
    base_config = getattr(configs, config_name)
    config = configs.remove_wait_times(base_config)

    if version:
        config['version'] = version

    if r or stdout:
        csvstring = StringIO()
        csvwriter = csv.writer(csvstring)
    else:
        if pdf:
            fig = plt.figure(dpi=300)
        else:
            fig = plt.figure()

    if pdf:
        latexify()

    filename = config_name.lower()


    if tp:
        data = network.get_plot_data(config, ["results->'avg_throughput'", "results->'incidents'", "config->>'name'"])
        logger.info("Got Data")

        if max_tp:
            data = [(i, x, y, name) for i,x,y,name in data if int(re.findall('\d+', name)[0]) <= max_tp]

        logger.info('%d points', len(data))

        ids, x, y, name = zip(*data)



        if r or stdout:
            csvwriter.writerow(['x', 'y'])
            for row in zip(x, y):
                csvwriter.writerow(row)
        else:
            #fig.suptitle(runid)
            ax = fig.add_subplot(111)

            def onpick3(event):
                ind = event.ind
                for i in ind:
                    print(ids[i])
                    #get_result(ids[i])

            x = np.array(x)
            y = np.array(y)

            n = 1000
            #distance_weight = 0.01

            # def rolling_avg(ax):
            #     indexes = np.logical_and(x > (ax-n), x < (ax + n))
            #     values = y[indexes]
            #     #x_values = x[indexes]
            #     #weights = 1/np.abs((x_values - ax) + 1)
            #     return np.average(values)

            # x2 = np.arange(2000, 14000, 10)
            # y2 = []
            # for x_val in x2:
            #     y2.append(rolling_avg(x_val))

            # ax.plot(x2, y2)

            ax.scatter(x, y, picker=True)
            #sns.regplot(x, y, lowess=True, y_jitter=0.3, x_jitter=0, ax=ax, scatter_kws={'picker': True})
            fig.canvas.mpl_connect('pick_event', onpick3)
            #plt.ylim(-1, 12)
            plt.xlabel('Throughput')
            plt.ylabel('Incidents')

            plt.tight_layout()

    elif name:
        filename += '.names'
        data = network.get_plot_data(config, ["config->>'name'"])
        logger.info('%d points', len(data))

        ids, names = zip(*data)
        counter = Counter()
        for name in names:
            counter[name] += 1

        counts = counter.most_common()
        counts = [(int(re.findall('\d+', name)[0]), name, cnt) for name, cnt in counts]

        counts.sort()

        width = .8

        numbers, names, counts = zip(*counts)
        ind = np.arange(len(names))

        plt.title(config['name'])

        plt.bar(ind, counts, width=width)
        plt.xticks(ind+width / 2, numbers)
        #plt.ylim(24, 30)
        plt.tight_layout()

    elif wtp:
        filename +='.wanted_throughputs'
        data = network.get_plot_data(config, ["config->>'name'", "results->'avg_throughput'"])
        logger.info('%d points', len(data))

        ids, names, throughputs = zip(*data)
        inputs = [int(re.findall('\d+', name)[0]) for name in names]

        plt.title(config['name'])


        plt.scatter(inputs, throughputs)

        #plt.ylim(24, 30)
        plt.tight_layout()
        plt.show()

    filename = filename + '.pdf'

    if r:
        if not os.path.exists('pdf'):
            os.makedirs('pdf')
        r_process = subprocess.Popen(['Rscript', 'plot.R', os.path.join('pdf', filename)], stdin=subprocess.PIPE)
        r_process.communicate(csvstring.getvalue())
    elif stdout:
        print csvstring.getvalue()
    elif pdf:
        ax = plt.gca()
        format_axes(ax)
        if not os.path.exists('pdf'):
            os.makedirs('pdf')
        plt.savefig(os.path.join('pdf', filename))
    else:
        plt.show()

if __name__ == '__main__':
    main()
