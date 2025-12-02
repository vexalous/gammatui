# gammatui

A lightweight, highly customizable text user interface to adjust gamma, brightness, and related effects for an output.

**Gammatui supports both X11 and Wayland (wlroots-based compositors).**

# Dependencies
For X11, ensure that `libx11-dev` and `libxrandr-dev` (or your distro's equivalent) are installed.

For Wayland, ensure you are using a supported compositor (like Hyprland or Sway) and that `libwayland-dev` is installed.

---

Then clone the github repository.
```bash
git clone https://github.com/vexalous/gammatui
```
Or, alternatively, download the zip and extract it.

---

Lastly, run make install and launch the application from an app launcher like rofi, or run make and use make run to launch the application from the source code folder.
