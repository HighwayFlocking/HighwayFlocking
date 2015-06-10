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

from copy import deepcopy

D_ToCity = 0
D_FromCity = 2

def merge_two_dicts(x, y):
    '''Given two dicts, merge them into a new dict as a deep copy.'''
    z = deepcopy(x)
    z.update(y)
    return z

DEFAULT = {
    'name': 'default',
    'warmup_time': 1 * 60,
    'run_time': 30 * 60,
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 3,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 1,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 1,
                'emergency': 0,
            }
        },
    ],
    'road_width': 2000,
}

ALL_SPAWNERS = merge_two_dicts(DEFAULT, {
    'name': 'symetric',
    'vary_throughput': (D_ToCity, D_FromCity),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1,2,3,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1,2,3,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ],
    })

SYMETRIC = merge_two_dicts(DEFAULT, {
    'name': 'symetric',
    'vary_throughput': (D_ToCity, D_FromCity),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1,2,3,6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1,2,3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ],
    })

TOO_MANY_EMERGENCY = merge_two_dicts(DEFAULT, {
    'name': 'too_man_emergency',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 3,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,3,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 1,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 1,
                'emergency': 0,
            }
        },
    ],
})

JUST_CARS = merge_two_dicts(DEFAULT, {
    'name': 'just_cars',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1,2,3,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        }
    ]
    })

CARS_AND_BUSSES = merge_two_dicts(DEFAULT, {
    'name': 'cars_and_busses',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 3,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,3,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ]
    })

ONCOMING = merge_two_dicts(DEFAULT, {
    'name': 'oncoming',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ],
    })

ONCOMING_AND_ONRAMP = merge_two_dicts(DEFAULT, {
    'name': 'oncoming_and_onramp',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ],
    })

ONCOMING_ONRAMP = ONCOMING_AND_ONRAMP

ONCOMING_ONRAMP_BUS = merge_two_dicts(DEFAULT, {
    'name': 'oncoming_onramp_bus',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 3,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 1,
                'emergency': 0,
            }
        },
    ],
    })

ONCOMING_ONRAMP_BUS_EMERGENCY = merge_two_dicts(DEFAULT, {
    'name': 'oncoming_onramp_bus_emergency',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 3,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 1,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 1,
                'emergency': 0,
            }
        },
    ],
    })

ONCOMING_ONRAMP_EMERGENCY = merge_two_dicts(DEFAULT, {
    'name': 'oncoming_onramp_emergency',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [2,4,5],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [3],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 1,
            }
        },
        {
            'direction': D_ToCity,
            'lanes': [6],
            'min_wait': 2.0,
            'max_wait': 2.5,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
        {
            'direction': D_FromCity,
            'lanes': [1],
            'min_wait': 20.0,
            'max_wait': 30.0,
            'vehicle_types': {
                'car': 5,
                'sedan': 5,
                'bus': 0,
                'emergency': 0,
            }
        },
    ],
    })

JUST_BUSSES = {
    'name': 'just_busses',
    'vary_throughput': (D_ToCity, ),
    'spawners': [
        {
            'direction': D_ToCity,
            'lanes': [1,2,4,5],
            'min_wait': 1.5,
            'max_wait': 2.0,
            'vehicle_types': {
                'car': 0,
                'sedan': 0,
                'bus': 1,
                'emergency': 0,
            }
        }
    ],
    'road_width': 2000,
}

ALL = {
    'name': '',
}

def througput(config, throughput):
    config = deepcopy(config)
    config['name'] = config['name'] + '+througput(%d)' % throughput

    for direction in config['vary_throughput']:
        cnt_spawners = sum([
                            len(s['lanes'])
                            for s in config['spawners']
                            if s['direction'] == direction])
        throughput_per_spawner = throughput / float(cnt_spawners)

        for spawner in config['spawners']:
            if not spawner['direction'] == direction:
                continue
            max_distance = 0.5
            min_wait = spawner['min_wait']
            avg_wait = 3600.0 / throughput_per_spawner
            max_wait = (avg_wait * 2) - min_wait
            if max_wait < (min_wait + max_distance):
                min_wait = avg_wait - (max_distance / 2)
                max_wait = avg_wait + (max_distance / 2)
            assert min_wait > 0
            assert max_wait >= min_wait + max_distance
            spawner['max_wait'] = max_wait
            spawner['min_wait'] = min_wait
    return config

# def vary_throughput_to_city(config, throughput=None):
#     if throughput is not None:
#         iterator = [throughput]
#     else:
#         iterator = np.arange(2000, 12000, 300)
#     for throughput in iterator:
#         config = deepcopy(config)
#         config['name'] = config['name'] + '+vary_throughput'

#         cnt_spawners = sum([
#                             len(s['lanes'])
#                             for s in config['spawners']
#                             if s['direction'] == D_ToCity])
#         throughput_per_spawner = throughput / float(cnt_spawners)

#         #from ipdb import set_trace; set_trace()

#         for spawner in config['spawners']:
#             if not spawner['direction'] == D_ToCity:
#                 continue
#             max_distance = 0.5
#             min_wait = spawner['min_wait']
#             avg_wait = 3600.0 / throughput_per_spawner
#             max_wait = (avg_wait * 2) - min_wait
#             if max_wait < (min_wait + max_distance):
#                 min_wait = avg_wait - (max_distance / 2)
#                 max_wait = avg_wait + (max_distance / 2)
#             assert min_wait > 0
#             assert max_wait >= min_wait + max_distance
#             spawner['max_wait'] = max_wait
#             spawner['min_wait'] = min_wait
#         yield config

def remove_wait_times(config):
    config = deepcopy(config)
    #del config['name'] # TODO: Remove this for delivery
    config['name'] = config['name'] + '+througput('
    #config['name'] = '' # TODO: Remove this for delivery
    if 'spawners' in config:
        for spawner in config['spawners']:
            if not spawner['direction'] in config['vary_throughput']:
                continue
            del spawner['max_wait']
            del spawner['min_wait']
        #del config['vary_throughput'] # TODO: Remove this when we are going to get the real stuff
    return config
