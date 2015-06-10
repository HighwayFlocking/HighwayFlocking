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

"""Run The Experiment Server.

Usage:
  server.py run
  server.py create_tables
  server.py (-h | --help)
  server.py --version

Options:
  -h --help           Show this screen.
  --version           Show version.
  --norepeat          Do not wrap everything in a while-loop
  --throughput VALUE  Just run with this value as throughput
  --existing          Connect to an existing simulator (implicates --norepeat)

"""

import os
import json
import os.path
import logging
import time
import msgpack
import cPickle as pickle
from distutils.version import StrictVersion

from flask import Flask, request, Response, send_file, abort
from flask.json import jsonify
from docopt import docopt
from werkzeug import secure_filename
from flask_httpauth import HTTPBasicAuth

import psycopg2
from psycopg2.extras import DictCursor
from psycopg2.extras import Json

from lib.server import db
import config

PATH = os.path.dirname(os.path.realpath(__file__))

def setup_logging():
    if not os.path.exists('log'):
        os.makedirs('log')
    logname = time.strftime('log/server_%Y.%m.%d_%H.%M.log')
    fh = logging.FileHandler(logname)
    fh.setLevel(logging.DEBUG)
    ch = logging.StreamHandler()
    ch.setLevel(logging.INFO)
    formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(funcName)s - %(message)s')
    fh.setFormatter(formatter)
    ch.setFormatter(formatter)
    logger = logging.getLogger('')
    logger.setLevel(logging.DEBUG)
    logger.addHandler(fh)
    logger.addHandler(ch)

setup_logging()


logger = logging.getLogger(__name__)

app = Flask(__name__)
auth = HTTPBasicAuth()

app.config['JSONIFY_PRETTYPRINT_REGULAR'] = False
app.config['UPLOAD_FOLDER'] = r'replays/'

users = {
    "worker": config.SECRET_KEY,
}

@auth.get_password
def get_pw(username):
    if username in users:
        return users.get(username)
    return None


# TODO: Cleanup


LOWEST_WORKER_VERSION = StrictVersion("0.2.1")


@app.route("/")
def hello():
    return "Hello World!"

@app.route('/get_tasks/')
@auth.login_required
def get_tasks():
    data = pickle.loads(request.get_data())
    get_all = data['get_all']
    return pickle.dumps(db.get_tasks(get_all), protocol=-1)

@app.route('/get_results/')
@auth.login_required
def get_results():
    data = pickle.loads(request.get_data())
    return pickle.dumps(db.get_results(data['config']), protocol=-1)

@app.route('/result/<int:resultid>/')
@auth.login_required
def get_result(resultid):
    return pickle.dumps(db.get_result(resultid), protocol=-1)

@app.route("/get_workers/", methods=['GET'])
@auth.login_required
def get_workers():
    return pickle.dumps(db.get_workers(), protocol=-1)

@app.route('/reset_task/', methods=['POST'])
@auth.login_required
def reset_task():
    data = pickle.loads(request.get_data())
    db.reset_task(taskid=data['taskid'])
    return 'ok'

@app.route("/add_task/", methods=['POST'])
@auth.login_required
def add_task():
    data = msgpack.unpackb(request.get_data())
    config = data['config']
    db.add_task(config)
    return 'ok'

@app.route("/add_tasks/", methods=['POST'])
@auth.login_required
def add_tasks():
    data = msgpack.unpackb(request.get_data())
    configs = data['configs']
    db.add_tasks(configs)
    return 'ok'

@app.route("/plot_data/", methods=['GET'])
@auth.login_required
def plot_data():
    data = pickle.loads(request.get_data())
    result = db.get_plot_data(data['config'], data['axes'])
    return pickle.dumps(result, protocol=-1)

@app.route("/get_task_for_completion/", methods=['POST'])
@auth.login_required
def get_task_for_completion():
    data = msgpack.unpackb(request.get_data())
    print data
    workername = data['workername']
    version = StrictVersion(data['version'])
    if version < LOWEST_WORKER_VERSION:
        task = dict(type='upgrade')
    else:
        task = db.get_next_task_for_completion(workername, data['version'])
        if task:
            task.update(type="simulator")
    return msgpack.packb(task)

@app.route("/task_status/", methods=['POST'])
@auth.login_required
def task_status():
    data = pickle.loads(request.get_data())
    worker = data['workername']
    taskid = data['taskid']
    percent_completed = data['percent_completed']
    db.task_status(taskid=taskid, worker=worker, percent_done=percent_completed)
    return 'ok'

@app.route("/task_completed/", methods=['POST'])
def task_completed():
    data = pickle.loads(request.get_data())
    worker = data['workername']
    taskid = data['taskid']
    config = data['config']
    results = data['results']
    db.task_completed(taskid, worker, config, results)
    return 'ok'

@app.route("/upload_replay/", methods=['POST'])
@auth.login_required
def upload_replay():
    taskid = int(request.form['taskid'])
    worker = request.form['workername']
    f = request.files['replay']
    if not os.path.exists(app.config['UPLOAD_FOLDER']):
        os.makedirs(app.config['UPLOAD_FOLDER'])
    filename = os.path.join(app.config['UPLOAD_FOLDER'], 'replay_%d.bin.gz' % taskid)
    logger.info("Saving replay to %s" % filename)
    f.save(filename)
    db.add_replay(taskid, filename, worker)
    return 'ok'

@app.route("/get_replay/<int:resultid>/")
@auth.login_required
def get_replay(resultid):
    filename = db.get_replay(resultid)
    if not filename:
        abort(404)
    logger.info("Sending file: %s" % filename)
    return send_file(filename)

@app.route("/worker.zip")
@auth.login_required
def download_worker():
    filename = os.path.join(PATH, 'worker.zip')
    logger.info("Sending file: %s" % filename)
    return send_file(filename)

@app.route("/cleanup/", methods=['POST'])
@auth.login_required
def cleanup():
    db.cleanup()
    return 'ok'

def main():
    arguments = docopt(__doc__, version='0.1')
    if arguments['create_tables']:
        db.create_tables()
    elif arguments['run']:
        app.run(host='0.0.0.0', threaded=True)

if __name__ == "__main__":
    main()
