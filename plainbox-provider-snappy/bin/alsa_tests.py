#!/usr/bin/env python3
# Copyright 2016 Canonical Ltd.
# All rights reserved.
#
# Written by:
#    Authors: Maciej Kisielewski <maciej.kisielewski@canonical.com>


import alsaaudio
import argparse
import math
import struct

RATE = 44100
PERIOD = 441  # chunk size (in samples) that will be used when talking to alsa


def sine(freq, length, period_len, amplitude=0.5):
    """
    Generate `period_len` samples of sine of `freq` frequency and `amplitude`.
    It generates at least `length` samples, totalling at next multiple of
    period_len. E.g. sine(440, 15, 10) will generate two 10-item lists of
    samples. Returns list of floats in the range of [-1.0 .. 1.0].
    """
    t = 0
    while t < length:
        sample = []
        for i in range(period_len):
            sample.append(
                amplitude * math.sin(2 * math.pi * ((t + i) / (RATE / freq))))
        yield sample
        t += period_len


class Player:
    def __init__(self):
        self.pcm = alsaaudio.PCM()
        self.pcm.setchannels(1)
        self.pcm.setformat(alsaaudio.PCM_FORMAT_FLOAT_LE)
        self.pcm.setperiodsize(PERIOD)

    def play(self, chunk):
        assert(len(chunk) == PERIOD)
        # alsa expects bytes, so we need to repack the list of floats into a
        # bytes sequence
        buff = b''.join([struct.pack("<f", x) for x in chunk])
        self.pcm.write(buff)


def playback_test(seconds):
    player = Player()
    for chunk in sine(440, seconds * RATE, PERIOD):
        player.play(chunk)


def main():
    actions = {
        'playback': playback_test
    }
    parser = argparse.ArgumentParser(description='Sound testing using ALSA')
    parser.add_argument('action', metavar='ACTION', choices=actions.keys())
    parser.add_argument('--duration', type=int, default=5)
    args = parser.parse_args()
    actions[args.action](args.duration)

if __name__ == '__main__':
    main()
