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

import logging

import psycopg2
from psycopg2.extras import DictCursor
from psycopg2.extras import Json
from flask import abort

import config

logger = logging.getLogger(__name__)

def connect():
    conn = psycopg2.connect(config.DB_CONNECTION_STRING, cursor_factory=DictCursor)
    return conn

def connect_plain():
    conn = psycopg2.connect(config.DB_CONNECTION_STRING)
    return conn

def create_tables():
    conn = connect()
    cur = conn.cursor()

    logger.info("Creating table workers if not exists")
    cur.execute("""CREATE TABLE IF NOT EXISTS workers (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            last_seen TIMESTAMP,
            doing VARCHAR(100) NULL,
            version VARCHAR(10) NULL,
            )
        """)

    logger.info("Creating table tasks if not exists")
    cur.execute("""CREATE TABLE IF NOT EXISTS tasks (
            id SERIAL PRIMARY KEY,
            request_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            started_time TIMESTAMP NULL,
            status_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            completed boolean DEFAULT FALSE NOT NULL,
            worker VARCHAR(100) NULL,
            config JSONB NOT NULL,
            percent_done float DEFAULT 0.0
            )
        """)

    logger.info("Creating table results if not exists")
    cur.execute("""CREATE TABLE IF NOT EXISTS results (
            id SERIAL PRIMARY KEY,
            taskid INT references tasks(id) UNIQUE,
            completed_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL,
            worker VARCHAR(100) NOT NULL,
            config JSONB NOT NULL,
            results JSONB NOT NULL,
            replay VARCHAR(255) NULL
            )
        """)

    conn.commit()
    cur.close()
    conn.close()

    logger.info("Creating tables completed")

def seen_worker(worker, conn, doing, version=None):
    cur = conn.cursor()
    cur.execute("""UPDATE workers SET last_seen=CURRENT_TIMESTAMP, doing=%s WHERE name=%s""",
        (doing, worker))
    if cur.rowcount == 0:
        cur.execute("""INSERT INTO workers (name, last_seen, doing) VALUES (%s, CURRENT_TIMESTAMP, %s)""",
            (worker, doing))

    if version:
        cur.execute("""UPDATE workers SET version=%s WHERE name=%s""",
            (version, worker))
    cur.close()

def get_workers():
    conn = connect()
    cur = conn.cursor()
    cur.execute("""SELECT id, name, (CURRENT_TIMESTAMP - last_seen) as last_seen, doing, version FROM workers ORDER BY name""")
    result = [dict(row.items()) for row in cur.fetchall()]
    cur.close()
    conn.close()
    return result

def get_tasks(get_all):
    conn = connect()
    cur = conn.cursor()
    if get_all:
        cur.execute("""SELECT * from tasks ORDER BY id""")
    else:
        cur.execute("""SELECT * from tasks WHERE completed=FALSE ORDER BY id""")
    result = [dict(row.items()) for row in cur.fetchall()]
    cur.close()
    conn.close()
    return result

def cleanup():
    conn = connect()
    cur = conn.cursor()
    cur.execute("""SELECT worker, count(id)
                   FROM tasks
                   WHERE completed=FALSE
                        AND started_time IS NOT NULL
                   GROUP BY worker
                   HAVING count(id) > 1""")
    workers = [row[0] for row in cur.fetchall()]
    for worker in workers:
        cur.execute("""SELECT started_time
                       FROM tasks
                       WHERE worker=%s
                       ORDER BY started_time DESC
                       LIMIT 1""", (worker, ))
        latest = cur.fetchone()[0]

        cur.execute("""UPDATE tasks
                       SET started_time=NULL, worker=NULL, completed=FALSE, percent_done=0
                       WHERE
                            worker=%s
                            AND started_time < %s
                            AND completed = FALSE""", (worker, latest))

    cur.execute("""UPDATE tasks
                   SET started_time=NULL, worker=NULL, completed=FALSE, percent_done=0
                   WHERE
                        started_time < CURRENT_TIMESTAMP - interval '3 hours'
                        AND completed = FALSE""")

    conn.commit()
    cur.close()
    conn.close()



def get_results(config):
    conn = connect()
    cur = conn.cursor()

    name = config['name'] + '%'
    del config['name']

    print name
    print config

    cur.execute("""SELECT
            id,
            taskid,
            completed_time,
            worker,
            config,
            results->'incidents' as incidents,
            results->'avg_throughput' as avg_throughput
        from results
        WHERE config @> %s AND config->>'name' LIKE %s""", (Json(config), name))
    result = [dict(row.items()) for row in cur.fetchall()]
    cur.close()
    conn.close()
    return result

def get_result(resultid):
    conn = connect()
    cur = conn.cursor()
    cur.execute("""SELECT * from results where id=%s""", (resultid, ))
    row = cur.fetchone()
    result = dict(row.items())
    cur.close()
    conn.close()
    return result

def reset_task(taskid):
    conn = connect()
    cur = conn.cursor()
    cur.execute("""DELETE FROM results WHERE taskid=%s""", (taskid, ))
    cur.execute("""UPDATE tasks SET started_time=NULL, worker=NULL, completed=FALSE, percent_done=0
        WHERE id=%s""", (taskid, ))
    conn.commit()
    cur.close()
    conn.close()

def add_task(config):
    conn = connect()
    cur = conn.cursor()

    cur.execute("""INSERT INTO tasks (config) VALUES (%s) RETURNING id""",
        (Json(config), ))
    new_id = cur.fetchone()[0]
    conn.commit()
    cur.close()
    conn.close()

    return new_id

def add_tasks(configs):
    conn = connect()
    cur = conn.cursor()

    parameters = [(Json(config), ) for config in configs]

    cur.executemany("""INSERT INTO tasks (config) VALUES (%s)""",
        parameters)
    conn.commit()
    cur.close()
    conn.close()

def get_plot_data(config, axes):
    logger.info("Get plot data, config=%s, axes=%s", config, axes)
    conn = connect_plain()
    cur = conn.cursor()
    name = config['name'] + '%'
    del config['name']
    rows = ','.join("%s" % axis for axis in axes)
    sql = "SELECT id, " + rows + """
        from results
        WHERE config @> %s AND config->>'name' LIKE %s"""
    print sql
    cur.execute(sql, (Json(config), name))
    result = [row for row in cur.fetchall()]
    cur.close()
    conn.close()
    return result

def get_next_task_for_completion(worker, version):
    conn = connect()
    cur = conn.cursor()
    seen_worker(worker, conn, 'get_task', version)
    # It is really important that this have "FOR UPDATE" to be sure that only one worker gets
    # ont task
    cur.execute("""SELECT id, config
        FROM tasks
        WHERE started_time IS NULL
        ORDER BY request_time, id
        LIMIT 1 FOR UPDATE""")
    row = cur.fetchone()
    if not row:
        task = None
    else:
        task = dict(row.items())

    if task:
        cur.execute("""UPDATE tasks
        SET
            started_time = CURRENT_TIMESTAMP,
            worker = %s
        WHERE id=%s""",
        (worker, task['id']))

    conn.commit()
    cur.close()
    conn.close()
    return task

def task_status(taskid, worker, percent_done):
    conn = connect()
    seen_worker(worker, conn, 'task_status')
    cur = conn.cursor()

    cur.execute("""SELECT started_time, completed, worker FROM tasks WHERE id=%s FOR UPDATE""", (taskid, ))
    task = cur.fetchone()
    if not task:
        logger.info("Could not find task %d, 404", taskid)
        abort(404)

    if not task['started_time']:
        logger.info("Task not started %d, 404", taskid)
        abort(404)

    if task['completed']:
        logger.info("Task alredy completed %d, 404", taskid)
        abort(404)

    if worker != task['worker']:
        logger.info("Wrong worker, %s!=%s for %d, 404", worker, task['worker'], taskid)
        abort(404)

    cur.execute("""UPDATE tasks
        SET percent_done = %s, status_time=CURRENT_TIMESTAMP
        WHERE id=%s AND worker=%s""", (percent_done, taskid, worker))
    conn.commit()
    cur.close()
    conn.close()

def task_completed(taskid, worker, config, results):
    conn = connect()
    seen_worker(worker, conn, 'task_completed')
    cur = conn.cursor()

    cur.execute("""SELECT * FROM tasks WHERE id=%s FOR UPDATE""", (taskid, ))
    task = cur.fetchone()
    if not task:
        abort(404)

    if not task['started_time']:
        abort(404)

    if task['completed']:
        abort(404)

    if worker != task['worker']:
        abort(404)


    cur.execute("""INSERT INTO results (taskid, worker, config, results)
        VALUES (%s, %s, %s, %s)""",
        (taskid, worker, Json(config), Json(results)))
    cur.execute("""UPDATE tasks SET completed = TRUE, percent_done=100
        WHERE id=%s""", (taskid, ))
    conn.commit()
    cur.close()
    conn.close()

def add_replay(taskid, replay, worker):
    conn = connect()
    seen_worker(worker, conn, 'add_replay')
    cur = conn.cursor()
    cur.execute("""UPDATE results SET replay = %s WHERE taskid=%s AND worker=%s""", (replay, taskid, worker))
    conn.commit()
    cur.close()
    conn.close()

def get_replay(resultid):
    conn = connect()
    cur = conn.cursor()
    cur.execute("""SELECT replay FROM results WHERE id=%s""", (resultid, ))
    row = cur.fetchone()
    if not row:
        return None
    result = row[0]
    conn.commit()
    cur.close()
    conn.close()
    return result
