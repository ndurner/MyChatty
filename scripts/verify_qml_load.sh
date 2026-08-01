#!/bin/zsh

set -euo pipefail

if (( $# != 1 )); then
    print -u2 "Usage: $0 /path/to/MyChatty.app"
    exit 64
fi

bundle_path=$1
executable_path="$bundle_path/Contents/MacOS/MyChatty"
if [[ ! -x "$executable_path" ]]; then
    print -u2 "MyChatty executable not found in bundle: $bundle_path"
    exit 66
fi

log_directory=$(mktemp -d "${TMPDIR:-/tmp}/mychatty-qml-load.XXXXXX")
trap 'rm -rf "$log_directory"' EXIT

if ! open -n -W -g --stdout "$log_directory/stdout" --stderr "$log_directory/stderr" \
    "$bundle_path" --args --verify-qml-load; then
    print -u2 "LaunchServices could not launch MyChatty."
    cat "$log_directory/stderr" >&2
    exit 1
fi

if rg -Fqx "MYCHATTY_QML_LOAD_OK" "$log_directory/stdout" "$log_directory/stderr"; then
    print "QML load check passed."
    exit 0
fi

print -u2 "QML load check failed: MyChatty did not confirm a root object was created."
cat "$log_directory/stdout" >&2
cat "$log_directory/stderr" >&2
exit 1
