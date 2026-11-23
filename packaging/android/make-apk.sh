#!/usr/bin/env bash
# Builds the Android app for arm64 phones and signs it with your release key:
#   build-android/QuantumChaos-<version>-arm64.apk
# The first build needs the note key, and for sync the server and its token, from the environment
# (the same values as the desktop build):
#   QUANTUMCHAOS_NOTE_KEY=<64 hex> QUANTUMCHAOS_SYNC_SERVER=https://host QUANTUMCHAOS_SYNC_TOKEN=<token> \
#       packaging/android/make-apk.sh
# Later builds reuse what build-android already has.
#
# Signing uses your keystore (KEYSTORE, default ~/.android/quantumchaos-release.jks; key alias
# KEY_ALIAS, default quantumchaos). apksigner asks for its password (or set KEYSTORE_PASSWORD).
# Make the keystore once with
#   keytool -genkeypair -keystore ~/.android/quantumchaos-release.jks -alias quantumchaos \
#           -keyalg RSA -keysize 4096 -validity 36500
# and keep it and its password safe: Android only updates the app with an APK signed by the same key.
#
# Where Qt and Android's tools are: QT_ANDROID_ARM64, QT_HOST_PATH, ANDROID_SDK_ROOT, ANDROID_NDK_ROOT
# (defaults: ~/Qt/6.11.2/android_arm64_v8a, ~/Qt/6.11.2/gcc_64, ~/Android/Sdk and its newest NDK).
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/../.." && pwd)"
build="${BUILD_DIR:-$root/build-android}"
qt="${QT_ANDROID_ARM64:-$HOME/Qt/6.11.2/android_arm64_v8a}"
host="${QT_HOST_PATH:-$HOME/Qt/6.11.2/gcc_64}"
sdk="${ANDROID_SDK_ROOT:-$HOME/Android/Sdk}"
ndk="${ANDROID_NDK_ROOT:-$(ls -d "$sdk"/ndk/* | sort -V | tail -1)}"
tools="$(ls -d "$sdk"/build-tools/* | sort -V | tail -1)"
keystore="${KEYSTORE:-$HOME/.android/quantumchaos-release.jks}"
alias="${KEY_ALIAS:-quantumchaos}"

if [ ! -f "$keystore" ]; then
    echo "No keystore at $keystore: make one first (see the top of this script)." >&2
    exit 1
fi

options=(-DCMAKE_BUILD_TYPE=Release "-DANDROID_SDK_ROOT=$sdk" "-DANDROID_NDK_ROOT=$ndk" "-DQT_HOST_PATH=$host")
[ -n "${QUANTUMCHAOS_NOTE_KEY:-}" ] && options+=("-DQUANTUMCHAOS_NOTE_KEY=$QUANTUMCHAOS_NOTE_KEY")
[ -n "${QUANTUMCHAOS_SYNC_SERVER:-}" ] && options+=("-DQUANTUMCHAOS_SYNC_SERVER=$QUANTUMCHAOS_SYNC_SERVER")
[ -n "${QUANTUMCHAOS_SYNC_TOKEN:-}" ] && options+=("-DQUANTUMCHAOS_SYNC_TOKEN=$QUANTUMCHAOS_SYNC_TOKEN")

echo "Building the APK in $build ..."
"$qt/bin/qt-cmake" -S "$root" -B "$build" -G Ninja "${options[@]}" > /dev/null
cmake --build "$build" --target apk

version="$(sed -n 's/^project(QuantumChaos VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")"
unsigned="$build/android-build/build/outputs/apk/release/android-build-release-unsigned.apk"
out="$build/QuantumChaos-$version-arm64.apk"
aligned="$(mktemp --suffix=.apk)"
trap 'rm -f "$aligned"' EXIT

echo "Signing with $keystore ($alias) ..."
"$tools/zipalign" -f -p 4 "$unsigned" "$aligned"
passwords=()
if [ -n "${KEYSTORE_PASSWORD:-}" ]; then
    passwords=(--ks-pass env:KEYSTORE_PASSWORD --key-pass env:KEYSTORE_PASSWORD)
fi
"$tools/apksigner" sign --ks "$keystore" --ks-key-alias "$alias" "${passwords[@]}" --out "$out" "$aligned"
"$tools/apksigner" verify "$out"

echo "Signed: $out"
echo "Install it on a phone connected over USB: adb install -r \"$out\""
