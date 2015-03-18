TartWM
===

A grid-based floating window manager with collision detection and tags.


Usage
---

Start TartWM with this at the end of your .xinitrc

    exec tartwm <path/to/config


Configuration
---

TartWM will read a config file from stdin. Here's an example:

    x 5
    y 6 
    gap 8
    top 16
    bw 2
    bf 00ccff
    bu 808080
    bi ac4142

This will create a 5×6 grid with 8px gaps and 16px of padding at the top edge of the
screen. The `y` option can be omitted for a 5×5 grid. There are `bottom`, `left` and
`right` options for the other screen edges.

`bw` sets the border width while `bf` and `bu` set the focused and unfocused border
colors, respectively. `bi` is the colour for windows marked urgent.


Control
---

TartWM is controlled entirely from the commandline and doesn't have any support for
key or mouse binding. You should really use something like sxhkd. 


Movement
---

    tartwm move [-c] [-w ID] [-d] x y

Moves the top left of the active window (or the window ID) to the coordinates `x` and `y`.
TartWM will automatically constrain this to the grid. If the `-c` flag is specified collision
detection will be applied and the window will shove other windows along with it. The `-d`
flag turns this into a delta, with the units for `x` and `y` in grid cells.

Examples:

    tartwm move -d 2 0

will move the active window 2 grid cells to the right.

    tartwm move -d 0 -1

will move the active window 1 grid cell up.


Resizing
---

    tartwm size [-c] [-w ID] [-d] x y

Works analogously to the move command. The `-c` flag in this case means windows that
get hit by the window being grown will be crushed, ie. they will shrink in response.


Anchoring
---

    tartwm anchor [-w ID]

Toggles anchoring of the active (or specified) window. Anchored windows can't be shoved
or crushed during moves/resizing.


Filling and packing
---

    tartwm fill [-w ID] [-w] [-h]

Makes the active (or specified) window fill the available space vertically (`-h`) or
horizontally (`-w`). This command also removes overlaps

    tartwm pack 

Will make all visible windows occupy as much space as possible without overlapping. Try
using it after shrinking a window or if you forgot to resize using `-c`.
Anchored windows will be ignored.

    tartwm grid

Give all visible windows equal size and width and arrange them in a grid. 



Focusing
---

    tartwm focus [-w ID] [-udlr] [-p]

With `-w`, focuses the specified window (try it with `dmenu` and `tartwm find`).
With one of `-udlr` focuses a window directionally.
With `-p` focuses the previous window.


Find
---

    tartwm find [-p x y] [string]

Searches for substring `string` in windows' titles and classes, returning the ID of
the most likely match. With flag `-p`, reports the ID of the window at coordinates
`x, y`.


Close
---

    tartwm close [-w ID]

Closes the active (or specified) window.


Tagging
---

Windows are assigned to tags, which can then be used to show/hide groups of windows.
Windows can only belong to one tag, but multiple tags can be shown on screen at once.
When resizing and moving, windows will only interact with other windows on their tag.
By default, newly spawned windows are assigned to the first tag.

    tartwm tag [-w ID] name

Assign the active (or specified) window to tag `name`.

    tartwm toggle tag

Shows or hides all the windows assigned to `tag`.


Listing information
---

    tartwm ls [-w ID]

Returns the title, tag and properties of all windows or with `-w` the specified
window. 


Saving and restoring layouts
---

    tartwm save

Saves the geometry of all windows (visible and hidden).

    tartwm restore

Restores the saved geometry.
