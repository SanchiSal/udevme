# udevme

A Qt 6 desktop application for managing udev rules for WebHID / hidraw device access in browsers (Brave, Chrome, Chromium, etc.).

![udevme](resources/udevme_icon.png)

## About

This application was created by **Sanchi** with AI assistance for full transparency. The goal of this project is to provide a fully open-source application that offers useful functionality for managing HID device permissions on Linux.

## Features

- **Device Discovery**: Automatically scans connected USB devices and identifies which support hidraw
- **Simple Rule Creation**: One-click rule creation for WebHID access
- **Edit & Notes**: Edit existing rules and add notes to remember why you created them
- **Rule Sync**: Reconciles system rules file with local config on startup
- **Theme Native**: Uses standard Qt widgets, inherits your system theme automatically

## Installation

### Prerequisites (Arch/CachyOS)

```bash
sudo pacman -S base-devel cmake qt6-base
```

### Build and Install

```bash
# Extract
tar -xzf udevme-1.0.0.tar.gz
cd udevme

# Build
./packaging/build.sh

# Install
./packaging/install.sh
```

### Uninstall

```bash
./packaging/uninstall.sh
```

## Usage

1. Launch udevme from your application menu or run `~/.local/bin/udevme/udevme`
2. Click **Add Rule**
3. Select the device(s) you want to enable for WebHID
4. Optionally select an app as a reminder of what this rule is for
5. Add any notes you want to remember about this rule
6. Click **Add**
7. Click **Apply** and enter your password
8. Unplug and replug your device

To edit an existing rule, double-click it or select it and click **Edit Rule**.

## How It Works

udevme creates rules in `/etc/udev/rules.d/99-udevme.rules` with the format:

```
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1234", ATTRS{idProduct}=="5678", MODE="0666", TAG+="uaccess", TAG+="seat"
```

This grants access to the hidraw device for WebHID in browsers.

## File Locations

| File | Location |
|------|----------|
| Executable | `~/.local/bin/udevme/udevme` |
| Configuration | `~/.local/bin/udevme/udevme.json` |
| Notes | `~/.local/bin/udevme/notes.json` |
| Staged Rules | `~/.local/bin/udevme/99-udevme.rules` |
| System Rules | `/etc/udev/rules.d/99-udevme.rules` |

## Troubleshooting

### Device not working after Apply

Unplug and replug the device, or run:
```bash
sudo udevadm trigger
```

### Check device permissions

```bash
ls -l /dev/hidraw*
```

### WebHID still not working

1. Ensure WebHID is enabled in your browser
2. Check `chrome://device-log` for errors
3. Try restarting the browser after applying rules

## License

MIT License
