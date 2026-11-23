#!/usr/bin/env bash
# Removes what install.sh put in place: the app, its icon and its launcher entry.
# Your notes and settings are kept in ~/.local/share/QuantumChaos (delete that yourself to remove them too).
# PREFIX=/some/folder packaging/linux/uninstall.sh removes an install made with the same PREFIX.
set -euo pipefail

prefix="${PREFIX:-$HOME/.local}"
id="io.github.chandandasgit.QuantumChaos"

for file in "$prefix/bin/quantumchaos" \
            "$prefix/share/icons/hicolor/scalable/apps/$id.svg" \
            "$prefix/share/applications/$id.desktop"; do
    if [ -e "$file" ]; then
        rm -f "$file"
        echo "Removed $file"
    fi
done

if command -v update-desktop-database > /dev/null && [ -d "$prefix/share/applications" ]; then
    update-desktop-database -q "$prefix/share/applications" || true
fi
if command -v gtk-update-icon-cache > /dev/null && [ -f "$prefix/share/icons/hicolor/index.theme" ]; then
    gtk-update-icon-cache -q "$prefix/share/icons/hicolor" || true
fi

echo "QuantumChaos is uninstalled. Your notes are still in ~/.local/share/QuantumChaos."
