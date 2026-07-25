[![build](https://github.com/rich-ayr/win11-toggle-rounded-corners/actions/workflows/build.yml/badge.svg)](https://github.com/rich-ayr/win11-toggle-rounded-corners/actions/workflows/build.yml)
[![License: MIT](https://img.shields.io/github/license/rich-ayr/win11-toggle-rounded-corners)](https://github.com/rich-ayr/win11-toggle-rounded-corners/blob/master/LICENSE)
[![Release](https://img.shields.io/github/v/release/rich-ayr/win11-toggle-rounded-corners?include_prereleases)](https://github.com/rich-ayr/win11-toggle-rounded-corners/releases)
[![Github All Releases](https://img.shields.io/github/downloads/rich-ayr/win11-toggle-rounded-corners/total.svg)](https://github.com/rich-ayr/win11-toggle-rounded-corners/releases)

# Win11 Toggle Rounded Corners

A simple utility to disable rounded window corners on Windows 11

[![](https://i.imgur.com/wqi0bh3.png)](https://i.imgur.com/wqi0bh3.png)

## Download

An installer as well as the standalone binary for portable use can be [**downloaded here**](https://github.com/rich-ayr/win11-toggle-rounded-corners/releases).

Windows 11 on x64 only. Some Anti-Virus/EDR products may block access to `dwm.exe`.

## Usage

The program requires **administrator** privileges and asks for them itself, so you get the usual UAC prompt when you start it. There is no need to open an elevated terminal first.

```
win11-toggle-rounded-corners --disable   # square corners
win11-toggle-rounded-corners --small     # small rounding
```

Run it with no arguments to toggle.

The patch is applied to `dwm.exe` in memory and is lost whenever `dwm.exe` restarts (logoff, reboot, or a crash).

The **installer** re-applies it with a logon task, so it persists; the **portable** binary is one shot.

## Reverting

How to undo the patch depends on how you installed it:

- **Portable:** Run the binary again with `--enable`, or just reboot.
- **Installer:** Uninstall it from **Settings → Apps**. This removes the scheduled task that re-applies the patch at every logon. Your current session will still show square corners until you reboot; after the reboot nothing re-applies the patch and rounded corners return on their own.

If a leftover task is still squaring corners after uninstalling, you can remove it manually from an elevated Command Prompt:

```
schtasks /Delete /F /TN "Run win11-toggle-rounded-corners as admin on logon"
```

## Build

You need clang-cl or MSVC with C++23/26 support, plus [meson](https://mesonbuild.com) and [ninja](https://ninja-build.org).

First clone the repo

```
git clone 'https://github.com/rich-ayr/win11-toggle-rounded-corners.git'
```

Then simply build it with meson

```
meson setup build
meson compile -C build
```
