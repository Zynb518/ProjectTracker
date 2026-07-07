#!/bin/sh
set -eu

mirror="${1:-}"
if [ -z "$mirror" ]; then
    echo "usage: $0 <apt-mirror-url>" >&2
    exit 1
fi

mirror="${mirror%/}"

rewrite_source_file() {
    source_file="$1"
    [ -f "$source_file" ] || return 0

    sed -i -E \
        "s#https?://(archive|security)\.ubuntu\.com/ubuntu#${mirror}#g" \
        "$source_file"
}

rewrite_source_file /etc/apt/sources.list

if [ -d /etc/apt/sources.list.d ]; then
    find /etc/apt/sources.list.d \
        -type f \( -name "*.list" -o -name "*.sources" \) \
        -print | while IFS= read -r source_file; do
            rewrite_source_file "$source_file"
        done
fi
