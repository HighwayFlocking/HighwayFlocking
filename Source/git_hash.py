#coding:utf-8

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

import subprocess
import os.path
import os

PATH = os.path.dirname(os.path.realpath(__file__))

os.chdir(PATH)

GIT_DIR = r'C:\Program Files (x86)\Git\bin\git.exe'

if not os.path.exists(GIT_DIR):
    git_hash = "unknown"
    print 'WARNING: Could not find git!'
else:
    git_hash = subprocess.check_output([GIT_DIR, 'rev-parse', 'HEAD'])
    git_hash = git_hash.strip()

template = """
#pragma once

#define GIT_HASH "{}"
#define GIT_HASH_LEN {}
"""

text = template.format(git_hash, len(git_hash))

if os.path.exists(os.path.join(PATH, 'HighwayFlocking', 'GitHash.h')):
    with open(os.path.join(PATH, 'HighwayFlocking', 'GitHash.h'), 'r') as f:
        current_text = f.read()
    if current_text == text:
        print 'Git hash already up to date'
        exit()

with open(os.path.join(PATH, 'HighwayFlocking', 'GitHash.h'), 'w') as f:
    f.write(text)

if git_hash != 'unknown':
    print 'Git hash saved to GitHash.h'
