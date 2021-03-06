# Base UI layer
# coded by Andy Friesen
# copyright whenever.  All rights reserved.
#
# This source code may be used for any purpose, provided that
# the original author is never misrepresented in any way.
#
# There is no warranty, express or implied on the functionality, or
# suitability of this code for any purpose.

# Frames hold an arbitrary number of widgets. (and are widgets themselves)
# Widgets are, currently, either text labels, or images.  Maybe more later.

import ika
import window

# globals (hurk)

defaultwindow = window.GradientWindow(ika.RGB(0, 0, 192), ika.RGB(0, 0, 128, 240), ika.RGB(0, 0, 64, 192), ika.RGB(0, 0, 100, 240))
defaultfont = None

def SetDefaultWindow(wnd):
    global defaultwindow
    defaultwindow = wnd

def SetDefaultFont(font):
    global defaultfont
    defaultfont = font

class Widget(object):
    "Basic widget interface."

    __slots__ = [
        'x',        #
        'y',        # Location of the widget, relative to its parent
        'width',    #
        'height',   # Size of the widget
        'border'    # How much breathing room to give the widget.  Only used for the Dock functions.
        ]

    def __init__(self, x = 0, y = 0, width = 0, height = 0):
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.border = 0

    def Draw(self, xoffset = 0, yoffset = 0):
        pass

    def DockLeft(self, wnd = None):
        self.x = self.border * 2
        if wnd is not None:
            self.x += wnd.Right
        return self

    def DockRight(self, wnd = None):
        x = wnd and wnd.Right or ika.Video.xres
        self.x = x - self.width - self.border * 2
        return self

    def DockTop(self, wnd = None):
        self.y = self.border * 2
        if wnd is not None:
            self.y += wnd.Bottom
        return self

    def DockBottom(self, wnd = None):
        y = wnd and wnd.Bottom or ika.Video.yres
        self.y = y - self.height - self.border * 2
        return self

    def get_Position(self):        return self.x, self.y
    def set_Position(self, pos):   (self.x, self.y) = pos

    def get_Size(self):            return self.width, self.height
    def set_Size(self, value):     (self.width, self.height) = value

    def get_Rect(self):            return (self.x, self.y, self.width, self.height)
    def set_Rect(self, r):         self.x, self.y, self.width, self.height = r

    Position = property(get_Position, set_Position, doc = 'Gets or sets the position')
    Size     = property(get_Size, set_Size, doc = 'Gets or sets the size (in pixels)')
    Rect     = property(get_Rect, set_Rect)

    def set_Left(self, value):      self.x = value
    def set_Right(self, value):     self.width = value - self.x
    def set_Top(self, value):       self.y = value
    def set_Bottom(self, value):    self.height = value - self.y

    Left     = property(lambda self: self.x, set_Left)
    Top      = property(lambda self: self.y, set_Top)
    Right    = property(lambda self: self.x + self.width, set_Right)
    Bottom   = property(lambda self: self.y + self.height, set_Bottom)

class Frame(Widget):
    "Base frame class.  A window, with things in it."

    __slots__ = Widget.__slots__ + [
        'wnd',      # The windowstyle to draw for this frame
        'widgets'   # List of child widgets
        ]

    def __init__(self, wnd = None, x = 0, y = 0, width = 0, height = 0):
        Widget.__init__(self, x, y, width, height)

        self.wnd = wnd or defaultwindow
        self.widgets = []
        self.border = int(1.5 * self.wnd.Left)     # or right or whatever

    def Draw(self, xoffset = 0, yoffset = 0):
        self.wnd.Draw(self.x, self.y, self.x + self.width, self.y + self.height)
        for x in self.widgets:
            x.Draw(self.x, self.y)

    def AutoSize(self):                 # makes the frame big enough to hold its contents
        self.width = 0
        self.height = 0

        for w in self.widgets:
            self.width = max(self.width, w.x+w.width)
            self.height = max(self.height, w.y+w.height)

    def AddChild(self, child):
        self.widgets.append(child)

    def RemoveChild(self, child):
        self.widgets.remove(child)

class TextFrame(Frame):
    "A frame with a simple text widget.  Nothing else."

    __slots__ = Frame.__slots__ + [
        'label'     # The TextLabel attached to this frame
        ]

    def __init__(self, wnd = None, x = 0, y = 0, width = 0, height = 0, font = None):
        Frame.__init__(self, wnd, x, y, width, height)
        self.label = TextLabel(font = font)
        self.widgets.append(self.label)

    def get_Text(self):
        return self.label.Text

    def set_Text(self, *value):
        self.label.Clear()
        self.label.AddText(list(value))

    Text = property(get_Text, set_Text)

    def Clear(self):
        self.label.Clear()
        self.label.Size = (0, 0)

    def AddText(self, args):
        self.label.AddText(args)
        self.Size = self.label.Size

    def AutoSize(self):
        self.label.AutoSize()
        self.Size = self.label.Size

    Text = property(lambda self: self.label.Text)


class TextLabel(Widget):
    'Textlabels hold one or more lines of text.'
    __slots__ = Widget.__slots__ + [
        'font',         # The font used to draw the text
        'ypage',        # Position of the viewport within the text list, if there is more text here than can be seen at once.
        'ymax',         # If nonzero, the bottom-line maximum height this label can reach.
        'text',         # The text itself.  Stored as a list of strings; one line per element.
        'PrintString'   # The method used to draw the text.  Typically for internal use only.
        ]

    def __init__(self, font = None, *text):
        Widget.__init__(self)

        self.font = font or defaultfont
        self.ypage = 0
        self.ymax = 0

        self.text = list(text)
        self.PrintString = None

        self.LeftJustify()

    Length = property(lambda self: len(self.text))

    def LeftJustify(self):
        self.PrintString = self.font.Print

    def RightJustify(self):
        self.PrintString = self.font.RightPrint

    def Center(self):
        self.PrintString = self.font.CenterPrint

    def Clear(self):
        self.text = []
        self.width = 0
        self.height = 0

    def SetText(self, *text):
        self.Clear()
        self.AddText(*text)

    def AddText(self, *text):
        for x in text:
            self.text.append(x)
            self.width = max(self.width, self.font.StringWidth(x))
        self.AutoSize()

    def Draw(self, xoffset = 0, yoffset = 0):
        curx = 0
        cury = 0

        i = self.ypage
        while cury < self.height and i < len(self.text):
            self.PrintString(self.x + xoffset + curx, self.y + yoffset + cury, self.text[i])
            i += 1
            cury += self.font.height

    def AutoSize(self):
        self.Size = (0, 0)

        for t in self.text:
            self.width = max(self.width, self.font.StringWidth(t))

        self.height = len(self.text) * self.font.height

        if self.ymax != 0:
            maxy = (self.ymax - 1) * self.font.height
        else:
            maxy = ika.Video.yres - self.y

        self.height = min(self.height, maxy)

    def set_YPage(self, value):
        self.ypage = min(value, self.height - self.height / self.font.height)

    def set_YMax(self, value):
        self.ymax = value
        self.AutoSize()

    YPage = property(lambda self: self.ypage, set_YPage)
    YMax = property(lambda self: self.ymax, set_YMax)
    Text = property(lambda self: self.text)

class ColumnedTextLabel(Widget):
    'A text label that holds one or more columns of text.  Columns stay lined up.'
    __slots__ = Widget.__slots__ + [
        'font',
        'columns',
        'columngap'
        ]

    #-------------------
    class Row(object):
        __slots__ = [
            'text',    # the text control that owns this row
            'row'      # ordinal index of the row within the text control
            ]

        def __init__(self, text, row):
            self.text = text
            self.row = row

        def __getitem__(self, idx):
            return self.text.column[idx].Text[row]

        def __setitem__(self, idx, value):
            self.text.column[idx].Text[row] = value
    #-------------------

    def __init__(self, x = 0, y = 0, columns = 1, font = None, columngap = 10):
        Widget.__init__(self, x, y)

        self.font = font or defaultfont
        self.columngap = columngap

        self.columns = []
        for i in range(columns):
            self.columns.append(TextLabel(font))

    Length = property(lambda self: self.columns[0].Length)

    # Fakes a 2D array.  mytextcontrol[row][column]
    def __getitem__(self, index):
        return Row(self, index)

    def LeftJustify(self):
        for c in self.columns:
            c.LeftJustify()

    def RightJustify(self):
        for c in self.columns:
            c.RightJustify()

    def Center(self):
        for c in self.columns:
            c.Center()

    def Clear(self):
        for c in self.columns:
            c.Clear()

    def AddText(self,*args):
        i = 0
        for c in self.columns:
            c.AddText(i < len(args) and args[i] or '')
            i += 1
        self.AutoSize()

    def Draw(self, xoffset = 0, yoffset = 0):
        for c in self.columns:
            c.Draw(self.x + xoffset, self.y + yoffset)

    def AutoSize(self):
        self.columns[0].AutoSize()
        self.columns[0].Position = (0, 0)
        for i in range(1, len(self.columns)):
            self.columns[i].AutoSize()
            self.columns[i].DockLeft(self.columns[i - 1])
            self.columns[i].x += self.columngap

        self.Size = (0, 0)
        for c in self.columns:
            self.width = max(self.width, c.Right)
            self.height = max(self.height, c.Bottom)

    def set_YPage(self, value):
        for c in self.columns:
            c.YPage = value

    def set_YMax(self, value):
        for c in self.columns:
            c.ymax = value  # dirty; should be using the property.  But I'm going to resize it in a minute anyway.

        self.AutoSize()

    YPage = property(lambda self: self.columns[0].YPage, set_YPage)
    YMax = property(lambda self: self.columns[0].ymax, set_YMax)

'''class ColumnedTextLabel(Widget):
    'A text label that holds one or more columns of text.  Columns stay lined up.'

    class __Row(object):
        'Represents a single row of the label'

        def __init__(self, parent, rowindex):
            self.parent = parent
            self.rowindex = rowindex

        def __len__(self):
            return len(self.parent.columns)

        def __getitem__(self, index):
            return self.parent.columns[index][rowindex]

        def __setitem__(self, index, value):
            self.parent.columns[index][rowindex] = value

    class __Column(object):
        'Represents a single column of the label'

        def __init__(self, parent, colindex):
            self.parent = parent
            self.colindex = colindex

        def __len__(self):
            return self.parent.numcolumns

        def __getitem__(self, index):
            return self.parent.columns[colindex][index]

        def __setitem__(self, index, value):
            self.parent.columns[colindex][index] = value

    def __init__(self, x = 0, y = 0, columns = 1, font = None, columngap = 10):
        Widget.__init__(self)
        self.font = font or defaultfont
        self.columngap = columngap
        self.numcolumns = columns
        self.columns = [[''] * columns]

    def Row(self, index):  return __Row(self, index)
    def Col(self, index):  return __Col(self, index)
'''


class Bitmap(Widget):
    "Bitmap widgets are images."
    __slots__ = Widget.__slots__ + [
        'img'   # The image drawn within this bitmap
        ]

    def __init__(self, img = None):
        Widget.__init__(self)

        self.img = img
        self.x = 0
        self.y = 0
        if img is not None:
            self.Size = img.width, img.height

    def set_Image(self, val):
        self.img = val
        if val is not None:
            self.Size = val.width, val.height

    Image = property(lambda self: self.img, set_Image)

    def Draw(self, xoffset = 0, yoffset = 0):
        # TODO: allow the widget to be resized, thus scaling the image?
        if self.img is not None:
            self.img.Blit(self.x+xoffset, self.y+yoffset)
