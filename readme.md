TartWM
===

A grid-based floating window utility with collision detection and tags.

After enjoying the pleasures of other, more full-featured window managers for
some time (eg. openbox), I've realised it would be more useful to have tartwm
able to run alongside such a window manager. The more powerful WM can then
handle pesky things like mouse focus and window decoration.

To this end, TartWM can be considered a simple but powerful tool for window 
arrangement. TartWM is not a static/dynamic tiler in the traditional sense.


Usage
---

Start TartWM with this at the end of your .xinitrc

    exec tartwm -c path/to/config


Configuration
---

TartWM will read a config file from stdin. Here's an example:

    x 5
    y 6 
    gap 8
    top 16

This will create a 5×6 grid with 8px gaps and 16px of padding at the top edge of the
screen. The `y` option can be omitted for a 5×5 grid. There are `bottom`, `left` and
`right` options for the other screen edges.


Control
---

TartWM is controlled entirely from the commandline and doesn't have any support for
key or mouse binding. You should really use something like sxhkd. 


Grid Conventions
---

TartWM does *everything* on a grid. Arguments to commands involving sizing and
movement (written as `GRID` below) are given in the following format:

    [±]X [[±]Y]

Prepending each argument with `+` or `-` makes it a relative value, while
omitting these make it absolute. Cell indexing starts at 1. Supplying a value
of 0 for X or Y will leave that coordinate unchanged.
 

Move
---

    tartwm move [-w ID] [-e] GRID

Moves the focused or specified window. If the window is blocked by another in
the direction it's trying to go, the offending window is shoved along with it.
Supplying the `-e` flag means the window will keep going until it hits something
(eg. another window or the screen edge.)

Examples:

    tartwm move +2

will move the active window 2 grid cells to the right.

    tartwm move 1 1

will move the window to the top left cell. If something is already in the cell,
the window won't move.

    tartwm move -e 0 -1

will move the window upwards until it hits something. Repeating the call and
omitting the `-e` flag will shove the blocking window (if possible) along.


Resize
---

    tartwm size [-w ID] [-e] [-d EDGE] GRID

Basically works the same way as moving the window. `EDGE` should be one (or more)
of `NSEW`. Supplying the `-e` flag will grow the window until it hits something. 
Absolute values will attempt to set the window to that size, or at least the 
maximum available size.

Examples:

    tartwm size -e -d W +1

will grow the left edge of the window until it hits something.

    tartwm size -d NE -1

will shrink the top and right edges of the window. 

    tartwm size 3 3

will attempt to set the window size to 3 cells wide and 3 cells tall. Only
unhindered edges can grow.
    

Anchor
---

    tartwm anchor [-w ID]

Toggles anchoring of the active (or specified) window. Anchored windows can't be
shoved or moved.


Arrange
---

    tartwm pack 

Will make all visible windows occupy as much space as possible without overlapping. Try
using it after shrinking a window or if you forgot to resize using `-c`.

    tartwm grid

Give all visible windows equal size and width and arrange them in the grid.
Anchored windows will be ignored.


Focus
---

    tartwm focus [-w ID] [-udlrp]

With `-w`, focuses the specified window (try it with `dmenu` and `tartwm find`).
With one of `-udlr` focuses a window directionally.
With `-p` focuses the previous window.


Find
---

    tartwm find [-p x,y] [-s string]

Searches for substring `string` in windows' titles and classes, returning the ID of
the most likely match. With flag `-p`, reports the ID of the window at coordinates
`x,y`. If both `-p` and `-s` are supplied, the substring acts as a filter, only
returning windows at `x,y` that contain `string`.


Close
---

    tartwm close [-w ID]

Closes the active (or specified) window, sending `WM_DELETE`.


Tag
---

Windows are assigned to tags, which can then be used to show/hide groups of windows.
Windows can only belong to one tag, but multiple tags can be shown on screen at once.
When resizing and moving, windows will only interact with other windows on their tag.
By default, newly spawned windows are assigned to the first tag.

    tartwm tag [-w ID] name

Assign the active (or specified) window to tag `name`.

    tartwm toggle tag

Shows or hides all the windows assigned to `tag`.

    tartwm solo tag

Will exclusively show windows assigned to `tag` and hide all others.


List Window Information
---

    tartwm ls [-w ID]

Returns the title, tag and properties of all windows or with `-w` the specified
window. 


Save and Restore Layouts
---

    tartwm save [-f file]

Saves the geometry of all windows (visible and hidden). If `-f` is supplied the
layout is saved to a file, otherwise it's just kept in memory.

    tartwm restore [-f file]

Restores the saved geometry from memory or the supplied file.
