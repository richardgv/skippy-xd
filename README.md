# Skippy-XD

...is a full-screen, expose-style, task-switcher for X11. You know that thing Mac OS X, Gnome 3, Compiz and KWin do where you press a hotkey and suddenly you see miniature versions of all your windows at once? Skippy-XD does just that.

Originally mirrored from [the Google Code project](https://code.google.com/p/skippy-xd/) this GitHub repo hosts the code of a fork from the original 0.5.0 release (2004), initially maintained by Nick Watts (2011) and Richard Grenville (2013).

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
git clone https://github.com/dreamcat4/skippy-xd.git
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

```sh
skippy-xd --stop-daemon
```

Once the daemon is running you can use the following command to activate it:

```sh
skippy-xd --activate-window-picker
```

However, sometimes pressing the Return key to run this last command also causes the window to be selected, so it is probably more effective in testing to do this:

```sh
sleep 0.5 && skippy-xd --activate-window-picker
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

There are 2 types of keyboard shortcuts. Global keyboard shortcuts, and the program's own keyboard shortcuts.


#### Global keyboard shortcuts

Firstly, there are global keyboard shortcuts, to be configured for invoking / launching Skippy. How to setup a global keyboard shortcut depends entirely on your chosen window manager. So we cannot provide you with specific instructions in that regard. But here are some links for a few of them:

* Xfce: http://wiki.xfce.org/faq#keyboard
* Openbox: https://urukrama.wordpress.com/openbox-guide/#Key_mouse
* Fluxbox: http://fluxbox-wiki.org/index.php/Keyboard_shortcuts#How_to_use_the_keys_file

It is recommended (best way) to set `skippy-xd --start-daemon` to be run after login. By adding it to the Startup Applications of your window manager. Then you should bind `skippy-xd --toggle-window-picker`, or `skippy-xd-toggle` to your preferred global keyboard shortcut. For most people, that will be `Alt+Tab` or `Super+Tab`.

Quite a few window managers already assign `Alt+Tab` (or `Super+Tab`) to something else. Like their own built-in Alt-Tab feature. So you might also need to figure out how to un-bind those pre-existing shortcuts, before you can re-assign them to Skippy-XD instead.

#### Program Keyboard Shortcuts

Once skippy is launched, it then has it's own set of keyboard shortcuts. These are fully configurable. You can specify your own values in your `skippy-xd.rc` configuration file. Should you need to override any of the defaults. To find the right key symbol names (keysyms) for regular keypress and/or key release events. For all the keybindings settings. Please use the program `xev` to probe for them. By typing / pressing the keys on your keyboard. Which is explained in the next section.

The sample config file `skippy-xd.sample.rc` shows all the default keyboard shortcuts. They in the `[bindings]` section, at the bottom of the file.

Normally, you will want them to be consistent with your chosen Global Keyboard Shortcut(s). As was described earlier (for invoking skippy). The default values have already been chosen to match program invocation via `Alt+Tab` and/or `Super+Tab`. This includes releasing the `Alt` key, or the `Super` key. Which will to select the currently highlighed item. As is consistent with `Alt-Tab` behaviour on other platforms.

Along with the above Key Bindings, there finally also a setting for specifying certain modifier keys. Which will reverse the direction od Alt-Tabbing. These can only be one of the special modifiers listed below. Rather than just any arbitrary key. Normally this function is mapped to either `Shift` and/or the `Control` modifier key. So those are the default. For example `Shift+Alt+Tab` will make skippy cycle backwards through the open windows. Again, the default value is chosen to be consistent with the expected Alt-Tabbing behaviour, as it is on other platforms.

However if you need to change it, then the masks which Skippy knows about / recognizes are listed near the bottom of `src/skippy.h` in a pair of lookup tables named `ev_modifier_mask` and `ev_modifier_mask_str`. Which were taken out from the original X windows header file `/usr/include/X11/X.h`. The current version of skippy knows about these X modifiers:

```c
static const int ev_modifier_mask[] = // { ... holds the lookup values }

static const char *ev_modifier_mask_str[] = \
{
	"ShiftMask",
	"LockMask",
	"ControlMask",
	"Mod1Mask",
	"Mod2Mask",
	"Mod3Mask",
	"Mod4Mask",
	"Mod5Mask",
	"Button1Mask",
	"Button2Mask",
	"Button3Mask",
	"Button4Mask",
	"Button5Mask",
	"AnyModifier",
	0x00
};
```

As it turns out, it's possible hold down a mouse button istead. As another type of modifier key. This is just to reverse the tabbing direction in skippy. If you wish to configure custom direction modifier(s), then you can just append them to the relevant setting `modifierKeyMasksReverseDirection`. Which is in the `[bindings]` section of your Skippy RC file.

#### How to identify keysyms with xev

Run the program `xev` in a terminal window. Then press the keys you are interested in. Observe the output.

***Example:***

Here is the name of the keysym on my keyboard for the Right Alt key. It is `ISO_Level3_Shift`. But it may be different for your keyboard.

```sh
KeyPress event, serial 38, synthetic NO, window 0x6000001,
    root 0x1e7, subw 0x0, time 29447789, (159,-18), root:(311,152),
    state 0x0, keycode 108 (keysym 0xfe03, ISO_Level3_Shift), same_screen YES,
    XKeysymToKeycode returns keycode: 92
    XLookupString gives 0 bytes:
    XmbLookupString gives 0 bytes:
    XFilterEvent returns: False

KeyRelease event, serial 38, synthetic NO, window 0x6000001,
    root 0x1e7, subw 0x0, time 29447853, (159,-18), root:(311,152),
    state 0x80, keycode 108 (keysym 0xfe03, ISO_Level3_Shift), same_screen YES,
    XKeysymToKeycode returns keycode: 92
    XLookupString gives 0 bytes:
    XFilterEvent returns: False
```

***Note 1:***

Key strings are case sensitive. So `alt_l` will not work - it will no match anything at all. You need to use the exact capitalization as it appears in the output from the `xev` program. For example `Alt_L` is a valid key name.

***Note 2:***

Be sure to separate with ` ` spaces each keybinding in a given setting. Like `keysLeft = Left b a h B A H` for example. You cannot use commas `, ; :` or other special characters in the keybindings settings in the `[bindings]` section of the skippy rc file. Otherwise it won't work.

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
