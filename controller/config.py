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

import os.path

PATH = os.path.dirname(os.path.realpath(__file__))

SIMULATOR_LOCATION = ""

# Search a few places for the simulator
SIMULATOR_LOCATIONS = [
    os.path.join(PATH, 'simulator', 'WindowsNoEditor', 'HighwayFlocking.exe'),
    os.path.join(PATH, 'HighwayFlocking.exe'),
    os.path.join(PATH, 'simulator', 'HighwayFlocking.exe'),
    os.path.join(PATH, '..', 'WindowsNoEditor', 'HighwayFlocking.exe'),
    os.path.join(PATH, '..', 'simulator', 'WindowsNoEditor', 'HighwayFlocking.exe'),
    os.path.join(PATH, '..', '..', 'simulator', 'WindowsNoEditor', 'HighwayFlocking.exe'),
]
for location in SIMULATOR_LOCATIONS:
    if os.path.exists(location):
        SIMULATOR_LOCATION = location
        break
SERVER_URL = "http://localhost:5000/"
SECRET_KEY = "SECRET"

# Search a few places for the editor
ENGINE_LOCATIONS = [
    "C:\\Program Files\\Epic Games\\4.7\\Engine\\Binaries\\Win64\\UE4Editor.exe",
    "C:\\Program Files (x86)\\Epic Games\\4.7\\Engine\\Binaries\\Win64\\UE4Editor.exe",
    "D:\\Program Files\\Epic Games\\4.7\\Engine\\Binaries\\Win64\\UE4Editor.exe",
    "D:\\Program Files (x86)\\Epic Games\\4.7\\Engine\\Binaries\\Win64\\UE4Editor.exe",
]

for location in ENGINE_LOCATIONS:
    if os.path.exists(location):
        ENGINE_LOCATION = location
        break

UPROJECT_LOCATION = os.path.join(PATH, '..', 'HighwayFlocking.uproject')

DB_CONNECTION_STRING = "dbname=highwayflocking user=highwayflocking password=password"

# Add secret key

try:
    from local_config import *
except ImportError:
    pass
