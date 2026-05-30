# KDE Web App Manager

A KDE Plasma application for creating and managing Progressive Web Apps (PWAs) on Fedora KDE.

Webapps are launched in a clean, browser-chrome-free window using Chromium's `--app=` mode or a custom Firefox profile, and are fully integrated into the KDE application launcher and task manager with their own icons.

![KDE Web App Manager](https://raw.githubusercontent.com/j4np0l/kwebappmanager/main/screenshots/main.png)

---

## Features

- **Add web apps** from any URL with a name, icon, and category
- **Fetch favicons** automatically from the site, or choose a custom image
- **Isolated profiles** — each app gets its own browser profile so cookies, logins, and settings don't bleed between apps or your main browser
- **KDE integration** — apps appear in KDE's application launcher and icons-only task manager with their own icon
- **Multi-browser support** — Chromium, Google Chrome, Brave, Microsoft Edge, and Firefox
- **Edit and remove** apps at any time; removal cleans up the desktop entry and profile

---

## Requirements

### Runtime

- KDE Plasma 6
- At least one supported browser:
  - Chromium / Google Chrome / Brave / Microsoft Edge (recommended — best PWA experience)
  - Firefox

### Build

- CMake ≥ 3.20
- Qt 6 (Core, Gui, Widgets, Network)
- KDE Frameworks 6: CoreAddons, I18n, XmlGui, WidgetsAddons, KIO, IconThemes, ConfigWidgets, Notifications
- Extra CMake Modules (ECM)

Install build dependencies on Fedora:

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qt6-qtbase-devel qt6-qtnetwork-devel \
  kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kxmlgui-devel \
  kf6-kwidgetsaddons-devel kf6-kio-devel kf6-kiconthemes-devel \
  kf6-kconfigwidgets-devel kf6-knotifications-devel
```

---

## Building

```bash
git clone https://github.com/j4np0l/kwebappmanager.git
cd kwebappmanager
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build -j$(nproc)
cmake --install build
```

The app installs to `~/.local/bin/kwebappmanager` and registers a desktop entry so it appears in the KDE application launcher.

---

## Usage

Launch from the app menu (**Web App Manager**) or run:

```bash
kwebappmanager
```

### Adding a web app

1. Click **Add Web App** (or press `Ctrl+N`)
2. Enter a name and URL
3. Choose a browser and category
4. Optionally click **Fetch from URL** to grab the site's favicon, or **Choose file…** for a custom icon
5. Click **OK**

The app is immediately available in the KDE application launcher.

### Launching

Double-click an entry in the list, press `Enter`, or click the **Launch** toolbar button.

### Editing / Removing

Select an entry and click **Edit** or **Remove** (or press `Delete`). Removing an app deletes its desktop entry and, if isolated profiles were enabled, its browser profile directory.

---

## How it works

| Browser | Launch mechanism | App-mode |
|---|---|---|
| Chromium / Chrome / Brave / Edge | `--app=<url> --class=<name> --user-data-dir=<profile>` | Native PWA window (no tabs or address bar) |
| Firefox | `--new-instance --profile <dir>` + `userChrome.css` | Toolbars hidden via CSS; KWin provides window decorations |

Each app gets a `.desktop` file written to `~/.local/share/applications/kwebapp-<id>.desktop` with a matching `StartupWMClass` so KDE's task manager can associate the browser window with the correct icon.

Isolated profiles are stored in `~/.local/share/kwebappmanager/profiles/<id>/`.

---

## License

GPL v3 — see [LICENSE](LICENSE).
