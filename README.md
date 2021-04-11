GNU Rocket OpenGL editor
========================

Beta Disclaimer
---------------

First a screenshot of how it looks:
![screenshot](/bin/rocketeditor_screen.png)

Regular disclaimer: Backup your stuff before using software :)

If you encounter any issues please try to report them at https://github.com/emoon/rocket/issues as
this would make my life much easier and I will try to fix them given time. Hopefully the editor should be useful even if still not 100% finalized bug wise.
Also the documentation hasn't been completed yet.

Here are the links to the current version of the editor:

* Mac: https://www.dropbox.com/s/toaiyfgzue9fm2v/RocketEditor1.2beta.zip?dl=0
* Windows: https://www.dropbox.com/s/ounkv6bcou5pgwe/RocketEditor1.2beta-Win.zip?dl=0

About
-----

RocketEditor is coded by Daniel 'emoon' Collin with documentation and testing by Heine 'bstrr' Gundersen.
Daniel Collin can be contacted at daniel aat collin dot com

Latest version of this program should be available at https://github.com/emoon/rocket

The SDL-based linux support is written by Konsta 'sooda' Hölttä at https://github.com/sooda/rocket, and later updated to use SDL2 by Lauri 'gustafla' Gustafsson at https://github.com/gustafla/rocket

Motivation
----------

About a year ago me and Heine 'bstrr' Gundersen (at Revision 2012) started to talk about that having an editor for GNU Rocket that worked on Mac OS X would be great as we both used Mac as our primary development platform.
This was the start of this another version of the editor. I (Daniel 'emoon' Collin) also wanted some features that weren't in the old editor (such as folding of tracks, having it cross platform, etc)
Some small work was started on the editor during spring and summer but not much happened. It was really during last autumn 2012 I started to work on it for real to get in the features I want to have.

Features:

* Includes (or should at least :) all features from the old editor (except track re-arrangement)
* More optimized for laptop keyboards (no usage of page up/down and similar keys)
* Group support. It's now possible to group channels together that makes sense (maybe a group for each part) The syntax for this is to name the tracks "mygroup:mytrack"
* Folding support for both groups and tracks. Something I found annoying in the old editor was that you had to scroll around quite a bit to get to channel you wanted. Now with folding you use the screen space better.
* Faster navigation: Can jump between key values, jump in step of 8 and also with bookmarks.
* Other features such as using more of the keyboard for biasing of values (see key layout below)
* Trackpad support (OS X only, mouse wheel elsewhere) to scroll around (can only be used with biasing)
* Cross platform. Read more about the code in the next section.
* Navigation while play-backing. It's now possible to jump forward/backward when playing the demo (demo "scratching")
* Fast way to insert a interpolated value by just pressing return on an empty row.

Source Code
-----------

Now uses a UI system called 'Emgui' which is an intermediate mode UI that was developed at the same time as the editor.
Emui uses OpenGL as the rendering API for the UI making it possible to port to many platforms but using a diffrent backend is also very possible.
The code is written in C and has currently been compiled with clang on Mac, Visual Studio 2012 (Express) on Windows, and gcc on Linux.
I have tried to keep the code as platform independent as possible and platform files are (mostly) in the src/macosx, src/windows and src/linux directories.
Some parts of the code could need some cleanup but should hopefully be quite easy to follow and understand.

Building the code
-----------------

All Mac, Windows and Linux versions use the Tundra build system (https://github.com/deplinenoise/tundra) so in order to build the code you need a tundra executable for your OS
I have made two of the available here for your convenience if you don't want to build the code yourself.

* Mac: https://dl.dropbox.com/u/5205843/tundra-mac.zip
* Windows: https://dl.dropbox.com/u/5205843/tundra-win.zip

Mac
---

To build on mac enter the rocket/ogl_rocket directory and run "tundra macosx-clang-debug" (or -release for release build) and in tundra-output you will find the app to run.
To use Xcode to build (or code in) use the XCode3-GenerateProj.sh file (works with Xcode 4.x also) and open the generated project in xcode (it's inside the tundra-output directory)

Windows
-------

Easiest is to run win32_gen_vs_solution.cmd and open the generated solution (it's in the tundra-output directory) in Visual Studio.
Note that you will need to setup your paths/variables so cl is included and can be started from command line. VS2012 Express seems to do this automatically but it may different depending on your compiler.
In that case run the "vsvars32.bat" included with Visual Studio from a command prompt and start Visual Studio from there.

If you want to build from command line that works just as fine as well (just set vsvars32 (as seen above) and run "tundra win32-msvc-debug" for the debug build.


```
git clone --depth=1 https://github.com/emoon/rocket
cd rocket
git submodule update --init --recursive
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" x86
"scripts/win32_gen_vs_solution.cmd"
"scripts/win32_launch_vs.cmd"
```

Linux
-----

The following was tested on Ubuntu 16.10 (yakkety).

Install SDL-dev:

```
sudo apt-get install libsdl1.2-dev
```

Install [tundra](https://github.com/deplinenoise/tundra):

```
git clone --depth=1 https://github.com/deplinenoise/tundra
cd tundra
make
sudo checkinstall make install
```

Install **rocket**. No project files are generated - Tundra builds the editor straight away.

```
git clone --depth=1 https://github.com/emoon/rocket
cd rocket
git submodule update --init --recursive
./linux-build.sh
```

Install `libbass.so` from the repo folder:

```
sudo cp external/bass/linux/libbass.so /usr/local/lib/
sudo chmod a+rx /usr/local/lib/libbass.so
sudo ldconfig
```

Start the basic example:

```
./t2-output/linux-gcc-release-default/basic_example
```

Start rocket:

```
./t2-output/linux-gcc-release-default/editor
```

Tips & tricks
-------------

* Fold away tracks or groups you are not currently working with using Alt+Left/Right and Ctrl+Alt+Left/Right

* Use Return to insert current interpolated value - also sets the interpolation type to what the previous key uses.

* A good way of bundling parameters for effects is by naming your tracks 'groupname:trackname', as this will group them together, thus making better tracknames than "logo-x-pos", "logo-y-pos", "title-x-pos", "title-y-pos". Groups can also be folded, so they don't take up screen estate when you are working on a different part of the demo.

* Insert bookmarks using the B key to easily jump between parts in the demo.

* To rename a track (or add it to a group), close the editor and open the .rocket xml file, which should be nicely formatted. Locate the track and rename it - then rename it in your demo.

* Bias using Alt+trackpad/scrollwheel. Add the Shift-key to bias even faster.

* Use selection to bias multiple keys - even across multiple tracks!

Keys
----

### OSX:

#### Editing:

Key | Action
--- | ---
0-9                 | Edit value
Esc                 | Cancel edit
QWERTY              | Bias selection +0.01, +0.1, +1, +10, +100, +1000
ASDFGH              | Bias selection -0.01, -0.1, -1, -10, -100, -1000
Alt+Trackpad scroll | Bias selection +1/-1
Shift+Alt+Trackpad  | Bias selection +10/-10
I                   | Toggle interpolation (step/linear/smooth/ramp)
Return              | Insert current interpolated value
Shift+Arrows        | Select
Cmd+T               | Select all keys in track
Cmd+X               | Cut
Cmd+C               | Copy
Cmd+V               | Paste
Cmd+Z               | Undo
Cmd+Shift+Z         | Redo
Delete              | Delete key

#### View:

Key | Action
--- | ---
Space               | Start/Stop
Arrows              | Move cursor around
Alt+Up/Down         | Jump 8 rows
Alt+Cmd+Up/Down     | Jump to StartRow/NextBookmark - PrevBookmark/EndRow
Cmd+Left/Right      | Jump to first/last track
Ctrl+Up/Down        | Jump to Previous/Next key in current track
Alt+Left/Right      | Fold/Unfold track
Ctrl+Alt+Left/Right | Fold/Unfold group
Trackpad scroll     | Scroll
B                   | Toggle Bookmark
Cmd+B               | Clear all Bookmarks
Tab                 | Jump between Row, Start Row, End Row, Track editing

#### Files:

Key | Action
--- | ---
Cmd+O               | Open file
Cmd+S               | Quicksave
Cmd+Shift+S         | Save as
Cmd+1/2/3/4         | Quickload recent files
Cmd+E               | Remote export

---------------------------------------------------------------------------------

### Windows and Linux:

#### Editing:

Key | Action
--- | ---
0-9                 | Edit value
Esc                 | Cancel edit
QWERTY              | Bias selection +0.01, +0.1, +1, +10, +100, +1000
ASDFGH              | Bias selection -0.01, -0.1, -1, -10, -100, -1000
I                   | Toggle interpolation (step/linear/smooth/ramp)
Return              | Insert current interpolated value
Shift+Arrows        | Select
Ctrl+T              | Select all keys in track
Ctrl+X              | Cut
Ctrl+C              | Copy
Ctrl+V              | Paste
Ctrl+Z              | Undo
Ctrl+Shift+Z        | Redo
Delete              | Delete key

#### View:

Key | Action
--- | ---
Space               | Start/Stop
Arrows              | Move cursor around
Alt+Up/Down         | Jump 8 rows
Ctrl+Alt+Up/Down    | Jump to StartRow/NextBookmark - PrevBookmark/EndRow
Ctrl+Left/Right     | Jump to first/last track
Ctrl+Up/Down        | Jump to Previous/Next key in current track
Alt+Left/Right      | Fold/Unfold track
Ctrl+Alt+Left/Right | Fold/Unfold group
Trackpad scroll     | Scroll
B                   | Toggle Bookmark
Ctrl+B              | Clear all Bookmarks
Tab                 | Jump between Row, Start Row, End Row, Track editing

#### Files:

Key | Action
--- | ---
Ctrl+O              | Open file
Ctrl+S              | Quicksave
Ctrl+Shift+S        | Save as
Ctrl+1/2/3/4        | Quickload recent files
Ctrl+E              | Remote export

