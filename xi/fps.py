# Simple FPS manager
# coded by Andy Friesen
# copyright whenever.  All rights reserved.
#
# This source code may be used for any purpose, provided that
# the original author is never misrepresented in any way.
#
# There is no warranty, express or implied on the functionality, or
# suitability of this code for any purpose.

import ika

class FPSManager(object):
    __slots__ = [
        'count'
        ]

    def __init__(_):
        _.count = ika.GetTime()

    def Render(_, func):
        _.count += 1 # What we wish the time was

        t = ika.GetTime()
        if t > _.count:     # behind schedule
            return

        if t == _.count:    # ahead of schedule.  Wait a second.
            ika.Delay(1)
            
        func()
        ika.Video.ShowPage()

