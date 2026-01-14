# fbxkb

![License](https://img.shields.io/badge/License-GPLv2-blue.svg)
![Version](https://img.shields.io/github/v/tag/aanatoly/fbxkb?label=version)

X11 keyboard indicator and switcher.

It shows a flag of current keyboard in a systray area and allows
with a click to switch to another one.

## Building

```bash
sudo apt install libgtk2.0-dev
PREFIX=$HOME/.local ./configure
make
make install
```

## FAQ

**Q1** How it knows what is current keyboard

It asks X server.

**Q2** How it knows what are alternative keyboards

It asks X server

**Q3** Why I have no alternative keyboards

Because your X server does not have them. Add them to it and fbxkb will update its view automatically.

**Q4** How can I add more keyboards to X server

You can use `setxkbmap` to do it. For example, to load 3 keyboards - English, German and Italian (us, de, it)
and to switch between them using both shifts, run this:

```bash
setxkbmap -option grp:switch,grp:shifts_toggle,grp_led:scroll us,de,it
```


