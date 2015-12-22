# Skippy-XD

...is a full-screen, expose-style, task-switcher for X11. You know that thing Mac OS X, Gnome 3, Compiz and KWin do where you press a hotkey and suddenly you see miniature versions of all your windows at once? Skippy-XD does just that.

Originally mirrored from [the Google Code project](https://code.google.com/p/skippy-xd/) this GitHub repo hosts the code of a fork from the original 0.5.0 release (2004). That fork was initially maintained by Nick Watts (2011) and is now maintained by [Richard Grenville](https://github.com/richardgv) (2013).

Skippy-XD is a standalone application for providing a window picker with live previews (including live video) on Linux desktops that run an X server with compositing support. That means it's not baked into the window manager, and compositing is used only when the window picker is active. 

The performance of your window manager isn't degraded, and you get a window picker that's every bit as elegant as OSX's Exposé or KWin's "Present Windows", with all the desktop-navigational efficiency.

**You can see a demo on [YouTube](http://www.youtube.com/watch?v=gVRPCd7OS38).**

## Installation

If you're using Ubuntu, you may simply install via the [Skippy-XD PPA.](https://launchpad.net/~landronimirc/+archive/skippy-xd) (or the [daily PPA](https://launchpad.net/~landronimirc/+archive/skippy-xd-daily/)).

```sh
sudo add-apt-repository ppa:landronimirc/skippy-xd
sudo apt-get update
sudo apt-get install skippy-xd
```

### From Source

To compile Skippy-XD from source you need to install the following development packages:

```sh
apt-get install libimlib2-dev libfontconfig1-dev libfreetype6-dev libx11-dev libxext-dev libxft-dev 
libxrender-dev zlib1g-dev libxinerama-dev libxcomposite-dev libxdamage-dev libxfixes-dev libxmu-dev
```

Now get the source, build and install:

```sh
git clone https://github.com/richardgv/skippy-xd.git
cd skippy-xd
make
make install
```

## Usage

### Config file

Download the original `skippy-xd.rc-default` config file and copy it to `~/.config/skippy-xd/skippy-xd.rc` and edit it to your liking.

### Command Line

Once Skippy-XD is installed, you can run it by entering `skippy-xd` at the command line. This will start the program, activate the window picker once, then exit. However, starting the program produces a brief flicker, so its better to keep the application running in the background as a daemon and just activate it when you want to use the window picker.

To run the daemon, use the following command:

```sh
skippy-xd --start-daemon
```

If for whatever reason you need to cleanly stop the running daemon, do this:

```
skippy-xd --stop-daemon
```

Once the daemon is running you can use the following command to activate it:

```
skippy-xd --activate-window-picker
```

However, sometimes pressing the Return key to run this last command also causes the window to be selected, so it is probably more effective in testing to do this:

```
sleep 1 && skippy-xd --activate-window-picker
This will wait 1 second, then activate the picker.
```

Alternatively, you can use the included toggle script, which starts the daemon, if it isn't already started, and activates the window picker:

```
skippy-xd-toggle
```

or, with the path to your configuration file:

```
skippy-xd-toggle /PATH/TO/YOUR/CONFIG
```

### Keyboard Shortcuts

Typically, it is helpful to set up keyboard shortcuts for skippy-xd. To get you started, here are some links:

* Xfce: http://wiki.xfce.org/faq#keyboard
* Openbox: https://urukrama.wordpress.com/openbox-guide/#Key_mouse
* Fluxbox: http://fluxbox-wiki.org/index.php/Keyboard_shortcuts#How_to_use_the_keys_file

A good method is to set `skippy-xd --start-daemon` to be run after login, and bind a key (like **SUPER_L** [windows key] or **F9**) to , `skippy-xd --toggle-window-picker`, or `skippy-xd-toggle`.

## Troubleshooting

Admittedly, skippy-xd is not yet perfect. Sometimes it will just stop working, but it doesn't seem to be a crash (`ps aux | grep skippy` shows the daemon is still running).  
To avoid this, your best option is to use the included toggle script:

```
skippy-xd-toggle
```

or, with the path to your configuration file:

```
skippy-xd-toggle /PATH/TO/YOUR/CONFIG
```

## See Also

* [Skippy-XD on Ubuntu Wiki](https://wiki.ubuntu.com/Skippy)
* [Skippy-XD on Google Code](https://code.google.com/p/skippy-xd/)
* [Original home of Skippy-XD](http://thegraveyard.org/skippy.html)
* [Skippy-XD on freecode](http://freecode.com/projects/skippy)
