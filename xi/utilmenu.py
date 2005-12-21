#!/usr/bin/env python

"""Miscellaneous utility menus for xi."""

# Coded by Andy Friesen
# Copyright whenever.  All rights reserved.

# This source code may be used for any purpose, provided that the
# original author is never misrepresented in any way.

# There is no warranty, express or implied on the functionality, or
# suitability of this code for any purpose.

import ika

from xi.menu import Menu, Cancel
from xi.party import party


class CharSelectMenu(Menu):

    def __init__(self, *args):
        super(CharSelectMenu, self).__init__(*args)
        self.Refresh()
        
    def Refresh(self):
        self.Clear()
        self.AddText(*[character.name for character in party])


def ChooseCharacter():
    charselect = CharSelectMenu()
    charselect.Refresh()
    charselect.Position = (((ika.Video.xres - charselect.width) / 2),
                           ((ika.Video.yres - charselect.height) / 2))
    trans.AddWindowReverse(charselect, (charselect.x, -charselect.height))
    trans.Execute()
    while True:
        result = charselect.Update()
        if result is not None:
            break
        ika.Map.Render()
        trans.Draw()
        ika.Video.ShowPage()
        ika.Input.Update()
    trans.AddWindow(charselect, (charselect.x, -charselect.height),
                    remove=True)
    trans.Execute()
    if result == Cancel:
        return None
    else:
        return party[result]
