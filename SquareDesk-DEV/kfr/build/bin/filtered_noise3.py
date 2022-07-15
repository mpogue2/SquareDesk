#!/usr/bin/env python
import sys
import os
sys.path.append(os.path.abspath(__file__ + '/../../../../dspplot/dspplot'))
sys.path.append(os.path.abspath(__file__ + '/../../../dspplot/dspplot'))
import dspplotting as dspplot

data = [
]
dspplot.plot(data, title='Filtered noise 3', div_by_N=True, file='../svg/filtered_noise3.svg')
