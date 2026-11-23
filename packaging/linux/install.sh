#!/usr/bin/env bash
# Builds QuantumChaos and installs it for the current user, without sudo:
#   ~/.local/bin/quantumchaos                                the app
#   ~/.local/share/icons/hicolor/scalable/apps/<app id>.svg  its icon
#   ~/.local/share/applications/<app id>.desktop             its launcher entry
# PREFIX=/some/folder packaging/linux/install.sh installs under that folder instead of ~/.local.
# The first build needs the note key, and for sync the server and its token, from the environment:
#   QUANTUMCHAOS_NOTE_KEY=<64 hex> QUANTUMCHAOS_SYNC_SERVER=https://host QUANTUMCHAOS_SYNC_TOKEN=<token> \
#       packaging/linux/install.sh
# Later builds reuse what the build folder already has. The app needs Qt 6 and OpenSSL (for HTTPS)
# from the system at run time.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
prefix="${PREFIX:-$HOME/.local}"
id="io.github.chandandasgit.QuantumChaos"
build="$root/build-release"

options=(-DCMAKE_BUILD_TYPE=Release)
[ -n "${QUANTUMCHAOS_NOTE_KEY:-}" ] && options+=("-DQUANTUMCHAOS_NOTE_KEY=$QUANTUMCHAOS_NOTE_KEY")
[ -n "${QUANTUMCHAOS_SYNC_SERVER:-}" ] && options+=("-DQUANTUMCHAOS_SYNC_SERVER=$QUANTUMCHAOS_SYNC_SERVER")
[ -n "${QUANTUMCHAOS_SYNC_TOKEN:-}" ] && options+=("-DQUANTUMCHAOS_SYNC_TOKEN=$QUANTUMCHAOS_SYNC_TOKEN")

echo "Building QuantumChaos (Release) in $build ..."
cmake -S "$root" -B "$build" -G Ninja "${options[@]}" > /dev/null
cmake --build "$build" --target appQuantumChaos

echo "Installing under $prefix ..."
install -Dm755 "$build/appQuantumChaos" "$prefix/bin/quantumchaos"
install -Dm644 "$root/resources/icons/icon.svg" "$prefix/share/icons/hicolor/scalable/apps/$id.svg"
# The launcher runs the installed app by its full path, so ~/.local/bin need not be on PATH.
entry="$(mktemp)"
trap 'rm -f "$entry"' EXIT
sed "s|^Exec=.*|Exec=$prefix/bin/quantumchaos|" "$here/$id.desktop" > "$entry"
install -Dm644 "$entry" "$prefix/share/applications/$id.desktop"

if command -v update-desktop-database > /dev/null; then
    update-desktop-database -q "$prefix/share/applications" || true
fi
if command -v gtk-update-icon-cache > /dev/null; then
    gtk-update-icon-cache -q -t "$prefix/share/icons/hicolor" || true
fi

echo "Installed. Start it from your launcher (QuantumChaos) or run $prefix/bin/quantumchaos"
