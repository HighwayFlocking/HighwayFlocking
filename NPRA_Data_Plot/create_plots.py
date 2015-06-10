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

from datetime import date
from math import sqrt
from pprint import pprint

import pandas as pd
import numpy as np
import matplotlib
matplotlib.use('PDF')
import matplotlib.pyplot as plt

SPINE_COLOR = 'gray'

def latexify(fig_width=None, fig_height=None):
    """Set up matplotlib's RC params for LaTeX plotting.
    Call this before plotting a figure.

    Parameters
    ----------
    fig_width : float, optional, inches
    fig_height : float,  optional, inches
    """

    # code adapted from http://www.scipy.org/Cookbook/Matplotlib/LaTeX_Examples

    # Width and max height in inches for IEEE journals taken from
    # computer.org/cms/Computer.org/Journal%20templates/transactions_art_guide.pdf

    matplotlib.rcdefaults()

    if fig_width is None:
        fig_width = 6

    if fig_height is None:
        golden_mean = (sqrt(5)-1.0)/2.0    # Aesthetic ratio
        fig_height = fig_width*golden_mean # height in inches

    MAX_HEIGHT_INCHES = 8.0
    if fig_height > MAX_HEIGHT_INCHES:
        print("WARNING: fig_height too large:" + fig_height +
              "so will reduce to" + MAX_HEIGHT_INCHES + "inches.")
        fig_height = MAX_HEIGHT_INCHES

    params = {'backend': 'pdf',
              'text.latex.preamble': ['\\usepackage{gensymb}'],
              'axes.labelsize': 8, # fontsize for x and y labels (was 10)
              'axes.titlesize': 8,
              'text.fontsize': 8, # was 10
              'legend.fontsize': 8, # was 10
              'xtick.labelsize': 8,
              'ytick.labelsize': 8,
              'text.usetex': True,
              'figure.figsize': [fig_width,fig_height],
              'font.family': 'serif',
              'lines.linewidth': 0.3,
              'patch.linewidth': 0.2,
              #'dpi': 300,
    }

    pprint(params)

    matplotlib.rcParams.update(params)


def format_axes(ax):

    for spine in ['top', 'right']:
        ax.spines[spine].set_visible(False)

    # for spine in ['left', 'bottom']:
    #     ax.spines[spine].set_color(SPINE_COLOR)
    #     ax.spines[spine].set_linewidth(0.5)

    ax.xaxis.set_ticks_position('bottom')
    ax.yaxis.set_ticks_position('left')

    # for axis in [ax.xaxis, ax.yaxis]:
    #     axis.set_tick_params(direction='out', color=SPINE_COLOR)

    # return ax

colors = [
    (31, 119, 180),
    (255, 127, 14)
]

colors = [(r/255., g/255., b/255.) for r, g, b in colors]

def read_data():
	data = pd.read_csv('LysakerLokket 8mars til 15 mars.csv', sep=';', decimal=',', parse_dates=[0], index_col=0, usecols=[0, 3, 4, 5, 6, 8])
	data = data.sort()
	return data

def plot_speeds(all_data):
	#for day in range(8, 16):
	latexify()
	for day in [12]:
		date = '2015-03-%02d' % day
		print '    Plotting for %s...' % date
		data = all_data[date]

		f, (ax1, ax2) = plt.subplots(2, sharex=True, dpi=600)

		ax1.set_ylim(0, 100)
		ax2.set_ylim(0, 1800)
		ax1.set_ylabel('speed (km/h)')
		ax2.set_ylabel('cars per hour per lane')
		pd.rolling_mean(data[(data['lane'] == 2) | (data['lane'] == 4)].speed.resample('1Min', fill_method='ffill'), window=10).plot(label='speed to Oslo', ax=ax1)
		#data[(data['lane'] == 2) | (data['lane'] == 4)].gap.resample('5Min').plot(label='gap to Oslo')
		pd.rolling_mean((data[(data['lane'] == 2) | (data['lane'] == 4)].speed.resample('1Min', how='count')*60/2), window=10).plot(label='number of cars to Oslo', ax=ax2)
		pd.rolling_mean(data[(data['lane'] == 1) | (data['lane'] == 3)].speed.resample('1Min', fill_method='ffill'), window=10).plot(label='speed from Oslo', ax=ax1)
		#data[(data['lane'] == 1) | (data['lane'] == 3)].gap.resample('5Min').plot(label='gap from Oslo')
		pd.rolling_mean((data[(data['lane'] == 1) | (data['lane'] == 3)].speed.resample('1Min', how='count')*60/2), window=10).plot(label='number of cars from Oslo', ax=ax2)
		#for lane in range(1, 5):
		#	data[data['lane'] == lane].count.resample('5Min', how='sum').plot(label='#vehicles lane %d' % lane)
		ax1.legend(loc='best')
		ax2.legend(loc='best')
		ax2.set_xlabel('time')
		#plt.tight_layout()
		format_axes(ax1)
		format_axes(ax2)
		plt.savefig('%s.pdf' % date)
		plt.clf()
		plt.close()

if __name__ == '__main__':
	print "Reading data..."
	data = read_data()
	print "Plotting data..."
	plot_speeds(data)
