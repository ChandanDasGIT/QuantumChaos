# QuantumChaos

A notes app in the style of Google Keep for Linux, Android and Windows, with a Qt 6 / QML
interface and a pure C++ backend.

- A grid of notes with pins, colours, labels, checklists, archive and trash, and search
- An editor that saves as you type, with undo / redo and automatic list numbering
  (`1.`, `a)`, `-`, `•` continue on Enter)
- Every note is stored **encrypted** (XChaCha20-Poly1305, Monocypher) with a key that is built
  into your own copy of the app
- Optional **sync** with your own server: notes travel and are stored there encrypted, edits made
  on two devices are merged part by part, and changes from your other devices show up at once,
  even in a note you have open
- Light and dark themes; a phone layout

## The note key

Each build carries a 256-bit key that encrypts every note, on the device and on the sync server.
It is given when the build is configured and is never in the source code.

- Make it once and **keep a copy** (a password manager): notes written with one key can't be read
  with another.
- Use **the same key for every build, on every device**, so they can read each other's notes.
- Don't share your builds (APK, binaries): the key and the sync token are inside them.

Make a key with either of:

```sh
openssl rand -hex 32
od -An -tx1 -N32 /dev/urandom | tr -d ' \n'; echo
```

## Sync (optional)

Sync needs a QuantumChaos sync server (a separate program you host yourself) and its token. Give
both when configuring; without them the app works offline and sync stays off. The server only
ever sees encrypted notes. It answers over HTTPS:

| Request | What it does |
| --- | --- |
| `GET /changes?since=<rev>[&wait=1]` | the notes changed since a revision (`wait=1`: waits up to 25 s) |
| `GET /notes/<id>` | one note, encrypted |
| `PUT /notes/<id>` with `X-Base-Rev` | stores a note (409 when another device changed it first) |
| `DELETE /notes/<id>` with `X-Base-Rev` | deletes a note for good |

Every request carries `Authorization: Bearer <token>`.

## Build requirements

QuantumChaos needs **Qt 6.10 or newer** (Quick, Network, Svg), CMake 3.21+, Ninja and a C++20
compiler.

### Fedora

```sh
sudo dnf install gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg
```

### Arch Linux

```sh
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-declarative qt6-svg
```

### Ubuntu and Debian

Their own Qt is too old (Ubuntu 26.04 has Qt 6.9, Debian 13 has Qt 6.8), so install Qt with
the [Qt Online Installer](https://www.qt.io/download-qt-installer-oss) (free Qt account): choose
Qt 6.10 or newer, *Desktop* (gcc 64-bit). Then the build tools and the libraries Qt needs:

```sh
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev libxkbcommon-dev libxcb-cursor0
```

and point CMake at that Qt before configuring (adjust the version):

```sh
export CMAKE_PREFIX_PATH=$HOME/Qt/6.11.2/gcc_64
```

## Building and installing

### Linux

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DQUANTUMCHAOS_NOTE_KEY=<your key> \
      -DQUANTUMCHAOS_SYNC_SERVER=https://<your server> -DQUANTUMCHAOS_SYNC_TOKEN=<your token>
cmake --build build
./build/appQuantumChaos
```

(In fish, leave out the `\` line breaks and write the values without `<` `>`.)

To install it for your user, with a launcher entry and icon (no root needed):

```sh
QUANTUMCHAOS_NOTE_KEY=<your key> QUANTUMCHAOS_SYNC_SERVER=https://<your server> \
QUANTUMCHAOS_SYNC_TOKEN=<your token> packaging/linux/install.sh
```

It installs `~/.local/bin/quantumchaos`; `packaging/linux/uninstall.sh` removes it again. Your
notes stay in `~/.local/share/QuantumChaos`. Later installs reuse the values in `build-release/`.

### Android

Requirements: Qt 6.10+ for *Android arm64-v8a* and *Desktop* from the Qt Online Installer; the
Android SDK (platform 36, build-tools 36) and NDK (27) from Android Studio's SDK Manager; a JDK
(17 or newer).

1. Make your signing key once, and keep it and its password: Android only updates the app with an
   APK signed by the same key.

   ```sh
   keytool -genkeypair -keystore ~/.android/quantumchaos-release.jks -alias quantumchaos \
           -keyalg RSA -keysize 4096 -validity 36500
   ```

2. Build and sign the APK (with the same note key as your other devices):

   ```sh
   QUANTUMCHAOS_NOTE_KEY=<your key> QUANTUMCHAOS_SYNC_SERVER=https://<your server> \
   QUANTUMCHAOS_SYNC_TOKEN=<your token> packaging/android/make-apk.sh
   ```

   It asks for the keystore's password and writes `build-android/QuantumChaos-<version>-arm64.apk`.
   Where Qt and the Android tools are is set with `QT_ANDROID_ARM64`, `QT_HOST_PATH`,
   `ANDROID_SDK_ROOT` and `ANDROID_NDK_ROOT` (see the top of the script for the defaults).

3. Install it: with the phone connected over USB (USB debugging on),
   `adb install -r build-android/QuantumChaos-*-arm64.apk`; or copy the APK to the phone, open it
   and allow installing from that app.

The notes live in the app's private storage. For sync, the phone must reach your server (for
example Tailscale on the phone).

### Windows

Not tested yet. Install Qt 6.10+ with the Qt Online Installer: *MinGW 64-bit*, plus *MinGW*,
*CMake* and *Ninja* under Developer and Designer Tools. In the "Qt 6.x (MinGW 64-bit)" command
prompt, from the project folder:

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DQUANTUMCHAOS_NOTE_KEY=<your key> ^
      -DQUANTUMCHAOS_SYNC_SERVER=https://<your server> -DQUANTUMCHAOS_SYNC_TOKEN=<your token>
cmake --build build
windeployqt --qmldir . build\appQuantumChaos.exe
build\appQuantumChaos.exe
```

`windeployqt` copies the Qt libraries next to the app, so the `build` folder then runs on its own.
The notes live in `%LOCALAPPDATA%\QuantumChaos`. Use the note key you made on Linux (it must be
the same everywhere).

## Tests

```sh
cmake --build build --target backendtests
./build/backendtests            # every test; ./build/backendtests notelist runs one group
```

The tests need no network and use throwaway folders.

## Layout

The code has three tiers, with the same folder names in each:

- `backend/`: plain C++20, no Qt (notes, storage, encryption, merging, sync), built as the
  `notesengine` library
- `wrapper/`: Qt C++ that hands the backend to QML
- `frontend/`: the QML interface

`dependencies/` holds Monocypher (CC0) and, for Android's HTTPS, OpenSSL (Apache 2.0).

## Licence

GPL-3.0; see [LICENSE](LICENSE).
