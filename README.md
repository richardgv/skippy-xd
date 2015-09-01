# Skippy-XD

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/richardgv/skippy-xd)

A full-screen expose-style task-switcher for X11.

Originally mirrored from [the Google Code project](https://code.google.com/p/skippy-xd/) this GitHub repo hosts the code of a fork from the original 0.5.0 release (2004). That fork was initially maintained by Nick Watts (2011) and is now maintained by [Richard Grenville](https://github.com/richardgv) (2013).

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

### Keyboard Shortcuts

Typically, it is helpful to set up keyboard shortcuts for skippy-xd. It is up to you to figure out how to set up keyboard shortcuts for your own window manager (it would be crazy to cover them all here). However, to get you started, here's some links:

* Xfce: http://wiki.xfce.org/faq#keyboard
* Openbox: https://urukrama.wordpress.com/openbox-guide/#Key_mouse
* Fluxbox: http://fluxbox-wiki.org/index.php/Keyboard_shortcuts#How_to_use_the_keys_file

A good method is to set `skippy-xd --start-daemon` to be run after login, and bind a key (like **SUPER_L** [windows key] or **F9**) to `skippy-xd --activate-window-picker`.  Then the flicker mentioned above only happens when starting the daemon on login, and you are free to use the hotkey you bound to open skippy-xd.

## Troubleshooting

Admittedly, skippy-xd is not yet perfect. I find that sometimes it will just stop working. I'm not sure why yet (or else I'd fix it!), but it doesn't seem to be a crash (`ps aux | grep skippy` shows the daemon is still running).  To work around it I currently have a script in my home folder called `restart-skippy-xd.sh` that contains this:

```
#!/bin/sh
killall skippy-xd
rm /tmp/skippy-xd-fifo
skippy-xd --start-daemon
```

Next, I bind this script to **CTRL+SHIFT+F9**. This means that when skippy-xd occasionally stops working, a quick **CTRL+SHIFT+F9**, then **F9** again and its back.

If you installed it from the Skippy-XD PPA, then you're advised to use skippy-xd-activate to activate the task-switcher (if the daemon is already running); the simple bash script will introduce a small delay before calling Skippy-XD, and this tends to increase reliability. If the plugin has already crashed, then use a different keyboard shortcut for skippy-xd-restart, another simple script that will clean up and restart Skippy-XD daemon (the same as `restart-skippy-xd.sh` above).

## See Also

* [Skippy-XD on Ubuntu Wiki](https://wiki.ubuntu.com/Skippy)
* [Skippy-XD on Google Code](https://code.google.com/p/skippy-xd/)
