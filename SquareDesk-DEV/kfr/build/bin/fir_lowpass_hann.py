#!/usr/bin/env python
import sys
import os
sys.path.append(os.path.abspath(__file__ + '/../../../../dspplot/dspplot'))
sys.path.append(os.path.abspath(__file__ + '/../../../dspplot/dspplot'))
import dspplotting as dspplot

data = [
                   0,
-0.0015258333572910198,
-0.011843352783028916,
-0.017968644835787476,
 0.01980547853830732,
 0.12142100441929311,
 0.24188034279985468,
 0.29646201043730425,
 0.24188034279985468,
 0.12142100441929327,
0.019805478538307462,
-0.017968644835787483,
-0.011843352783028923,
-0.0015258333572910304,
                   0,
]
dspplot.plot(data, phaseresp=True, phasearg='auto', title='15-point lowpass FIR, Hann window', file='../svg/fir_lowpass_hann.svg')
