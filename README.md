[![Github All Releases](https://img.shields.io/github/downloads/oberrich/win11-toggle-rounded-corners/total.svg)](https://github.com/oberrich/win11-toggle-rounded-corners/releases)

# Win11 Toggle Rounded Corners

A simple utility to disable rounded window corners on Windows 11

[![](https://i.imgur.com/5HIQf7n.png)](https://i.imgur.com/5HIQf7n.png)

## Download

An installer as well as the standalone binary for portable use can be [**downloaded here**](https://github.com/oberrich/win11-toggle-rounded-corners/releases).
The program requires **administrator** privileges.

Some Anti-Virus products may block the access to `dwm.exe`

## Usage

Run as administrator. The patch is applied to `dwm.exe` in memory, so it does **not** survive a restart of `dwm.exe` (logoff, reboot, or a `dwm.exe` crash) on its own.

```
win11-toggle-rounded-corners.exe --disable   # remove rounded corners
win11-toggle-rounded-corners.exe --enable    # restore rounded corners
```

The **installer** applies the patch and registers a logon scheduled task that re-applies it on each boot, so the change persists across restarts. The **portable** binary does a one-shot patch with no persistence.

## Reverting

How to undo the patch depends on how you installed it:

- **Portable:** Run the binary again with `--enable`, or just reboot — the patch doesn't survive a `dwm.exe` restart.
- **Installer:** Uninstall it from **Settings → Apps**. This removes the scheduled task that re-applies the patch at every logon. Your current session will still show square corners until you reboot; after the reboot nothing re-applies the patch and rounded corners return on their own.

If a leftover task is still squaring corners after uninstalling, you can remove it manually from an elevated Command Prompt:

```
schtasks /Delete /F /TN "Run win11-toggle-rounded-corners as admin on logon"
```

## Build

First clone the repo **recursive**ly

```
git clone --recursive 'https://github.com/oberrich/win11-toggle-rounded-corners.git'
```

Then simply build it with meson

```
meson setup build
meson compile -C build
```
