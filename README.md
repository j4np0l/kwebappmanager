# KDE Web App Manager

**Note:** I created (vibe coded) this because I wanted to have a clean way to create and manage PWAs on KDE. This is my first KDE app, Claude did most of the heavy lifting so there might be loads of IA slop code in here. You have been warned. In saying this, I would really appreaciate any sort of feedback if anyone happens to use this app.

A KDE Plasma application for creating and managing Progressive Web Apps (PWAs) on Fedora KDE.

Web apps run in a clean, chrome-free window powered by an embedded Qt WebEngine (Chromium) view, and are fully integrated into the KDE application launcher and task manager with their own icons. Links that lead off the app's own site open in your default browser, so the window stays focused on the app itself.

![KDE Web App Manager](https://raw.githubusercontent.com/j4np0l/kwebappmanager/main/screenshots/main.png)

---

## Features

- **Add web apps** from any URL with a name, icon, and category
- **Fetch favicons** automatically from the site, or choose a custom image
- **Embedded web view** — apps run in their own Qt WebEngine window with no tabs or address bar; no external browser needed
- **External links open in your default browser** — clicking a link to another site (or a `target=_blank` / `mailto:` link) hands off to your default browser instead of navigating the app away
- **Isolated profiles** — each app gets its own persistent profile so cookies, logins, and settings don't bleed between apps or your main browser
- **KDE integration** — apps appear in KDE's application launcher and icons-only task manager with their own icon
- **Edit and remove** apps at any time; removal cleans up the desktop entry and profile

---

## Requirements

### Runtime

- KDE Plasma 6
- Qt 6 WebEngine (`qt6-qtwebengine`) — provides the embedded Chromium view; no separate browser required

### Build

- CMake ≥ 3.20
- Qt 6 (Core, Gui, Widgets, Network, WebEngineWidgets)
- KDE Frameworks 6: CoreAddons, I18n, XmlGui, WidgetsAddons, KIO, IconThemes, ConfigWidgets, Notifications
- Extra CMake Modules (ECM)

Install build dependencies on Fedora:

```bash
sudo dnf install \
  cmake extra-cmake-modules \
  qt6-qtbase-devel qt6-qtnetwork-devel qt6-qtwebengine-devel \
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
3. Pick a category, and choose whether to use a dedicated (isolated) profile
4. Optionally click **Fetch from URL** to grab the site's favicon, or **Choose file…** for a custom icon
5. Click **OK**

The app is immediately available in the KDE application launcher.

### Launching

Double-click an entry in the list, press `Enter`, or click the **Launch** toolbar button.

### Editing / Removing

Select an entry and click **Edit** or **Remove** (or press `Delete`). Removing an app deletes its desktop entry and, if an isolated profile was enabled, its profile directory.

---

## How it works

Each app gets a `.desktop` file written to `~/.local/share/applications/kwebapp-<id>.desktop`. Instead of launching an external browser, its `Exec` line re-invokes the manager in PWA runtime mode:

```
kwebappmanager --app <id>
```

In this mode the app hosts the page itself in an embedded `QWebEngineView` (Qt WebEngine / Chromium) with no tabs or address bar. Because the manager owns the window, it can control navigation:

- **Same-site links** navigate within the app window (use `Alt+Left` / `Alt+Right` to go back/forward).
- **Off-site links**, `target=_blank` / `window.open` popups, and `mailto:` / `tel:` links are handed to your default browser via `QDesktopServices::openUrl()`.

The process sets its desktop file name to `kwebapp-<id>`, matching the `.desktop` entry's `StartupWMClass`, so KDE's task manager associates the window with the correct launcher and icon.

The embedded view's user agent has the `QtWebEngine/<version>` token stripped (leaving a plain Chrome user agent) so sites that sniff for a "real" browser don't reject it.

Isolated apps use a persistent WebEngine profile stored in `~/.local/share/kwebappmanager/profiles/<id>/`; non-isolated apps run off-the-record (nothing persists between launches).

---

## License

GPL v3 — see [LICENSE](LICENSE).
