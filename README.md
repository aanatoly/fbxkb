# fbxkb

![License](https://img.shields.io/badge/License-GPLv2-blue.svg)
![Version](https://img.shields.io/github/v/tag/aanatoly/fbxkb?label=version)

X11 keyboard indicator and switcher.

It shows a flag of current keyboard in a systray area and allows
with a click to switch to another one.

![ru](assets/xru.png)
![us](assets/xus.png)

## Usage

Edit `.xsession` (or `.xinitrc`) script to configure layouts and start `fbxkb`.
This example adds US English, German and Italian with both shifts switching.

```bash
setxkbmap -option grp:switch,grp:shifts_toggle,grp_led:scroll us,de,it
fbxkb &
```

## Building

```bash
sudo apt install libgtk2.0-dev
PREFIX=$HOME/.local ./configure
make
make install
```
