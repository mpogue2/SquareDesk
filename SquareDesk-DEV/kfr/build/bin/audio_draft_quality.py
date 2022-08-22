#!/usr/bin/env python
import sys
import os
sys.path.append(os.path.abspath(__file__ + '/../../../../dspplot/dspplot'))
sys.path.append(os.path.abspath(__file__ + '/../../../dspplot/dspplot'))
import dspplotting as dspplot

dspplot.plot(r'audio_draft_quality.wav', file='../svg/audio_draft_quality.svg')
