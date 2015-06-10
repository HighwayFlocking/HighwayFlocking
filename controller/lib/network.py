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

import gzip
import os
import cPickle as pickle
import logging
import os
import json

import requests
from requests_toolbelt.multipart.encoder import MultipartEncoder
from requests_toolbelt.downloadutils import stream
import msgpack
from requests.auth import HTTPBasicAuth

import config as cfg

logger = logging.getLogger(__name__)

auth = HTTPBasicAuth('worker', cfg.SECRET_KEY)

class StopWorkOnTask(Exception):
    pass

def add_task(config):
    data = dict(config=config)
    logger.info("Adding task %s" % config['name'])
    r = requests.post(cfg.SERVER_URL + "add_task/", data=msgpack.packb(data), auth=auth, timeout=60)
    r.raise_for_status()

def add_tasks(configs):
    data = dict(configs=configs)
    for config in configs:
        logger.info("Adding tasks %s" % config['name'])
    r = requests.post(cfg.SERVER_URL + "add_tasks/", data=msgpack.packb(data), auth=auth, timeout=60)
    r.raise_for_status()

def get_tasks(get_all):
    data = dict(get_all=get_all)
    r = requests.get(cfg.SERVER_URL + 'get_tasks/', data=pickle.dumps(data, protocol=-1), auth=auth, timeout=60)
    r.raise_for_status()
    return pickle.loads(r.content)

def get_results(config):
    data = dict(config=config)
    r = requests.get(cfg.SERVER_URL + 'get_results/', data=pickle.dumps(data, protocol=-1), auth=auth)
    r.raise_for_status()
    return pickle.loads(r.content)

def get_result(resultid):
    r = requests.get(cfg.SERVER_URL + 'result/%d/' % resultid, auth=auth, timeout=60)
    r.raise_for_status()
    return pickle.loads(r.content)

def get_workers():
    r = requests.get(cfg.SERVER_URL + 'get_workers/', auth=auth, timeout=60)
    r.raise_for_status()
    return pickle.loads(r.content)

def reset_task(taskid):
    data = dict(taskid=taskid)
    r = requests.post(cfg.SERVER_URL + 'reset_task/', data=pickle.dumps(data, protocol=-1), auth=auth, timeout=60)
    r.raise_for_status()

def get_plot_data(config, axes):
    data = dict(config=config, axes=axes)
    logger.info("Requesting plot data for %s", axes)
    r = requests.get(cfg.SERVER_URL + 'plot_data/', data=pickle.dumps(data, protocol=-1), auth=auth, timeout=60)
    r.raise_for_status()
    return pickle.loads(r.content)


def get_task_for_completion(workername, version):
    data = dict(workername=workername, version=version)
    r = requests.post(cfg.SERVER_URL + 'get_task_for_completion/', data=msgpack.packb(data), auth=auth, timeout=60)
    r.raise_for_status()
    return msgpack.unpackb(r.content)

def cleanup():
    r = requests.post(cfg.SERVER_URL + 'cleanup/', auth=auth, timeout=60)
    r.raise_for_status()

def post_status(workername, taskid, percent_completed):
    data = dict(
        workername=workername,
        taskid=taskid,
        percent_completed=percent_completed
        )

    r = requests.post(cfg.SERVER_URL + "task_status/", data=pickle.dumps(data, protocol=-1), auth=auth, timeout=60)
    if r.status_code == 404:
        raise StopWorkOnTask()
    r.raise_for_status()

def post_result(workername, taskid, config, results):
    data = dict(
        workername=workername,
        taskid=taskid,
        config=config,
        results=results
        )
    r = requests.post(cfg.SERVER_URL + 'task_completed/', data=pickle.dumps(data, protocol=-1), auth=auth, timeout=60)
    r.raise_for_status()

def post_replay(taskid, replayname, workername):
    logger.info("Gziping the replay")
    f_in = open(replayname, 'rb')
    f_out = gzip.open(replayname + '.gz', 'wb')
    f_out.writelines(f_in)
    f_out.close()
    f_in.close()

    logger.info("Uploading replay to server")

    with open(replayname + '.gz', 'rb') as f:
        m = MultipartEncoder(
        fields={'taskid': str(taskid),
                'workername': workername,
                'replay': ('replay.bin', f, 'application/gzip')
                }
        )

        #from ipdb import set_trace; set_trace()
        r = requests.post(cfg.SERVER_URL + 'upload_replay/', data=m, auth=auth, timeout=120,
                      headers={'Content-Type': m.content_type})
        r.raise_for_status()

        logger.info("Got response: %s" % r.content)

def download_replay(resultid, filename):
    r = requests.get(cfg.SERVER_URL + 'get_replay/%d/' % resultid, stream=True, auth=auth)
    r.raise_for_status()

    gz_filename = stream.stream_response_to_file(r, path=filename + '.gz')

    logger.info("GUnzipping the replay")
    f_in = gzip.open(gz_filename, 'rb')
    f_out = open(filename, 'wb')
    f_out.writelines(f_in)
    f_out.close()
    f_in.close()
    os.remove(gz_filename)

def download_worker():
    logger.info("Downloading updated worker")
    r = requests.get(cfg.SERVER_URL + 'worker.zip', stream=True, auth=auth, timeout=60)
    r.raise_for_status()
    logger.info("Getting the data")
    filename = stream.stream_response_to_file(r, path='worker.zip')
    logger.info("Download complete")
    return filename
