#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

set -euo pipefail

configuration="${1:-Release}"
package_name="${2:-inox-linux-x64}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

compiler_path="$repo_root/build/inox"
if [[ "$configuration" == "Release" && -x "$repo_root/build/Release/inox" ]]; then
    compiler_path="$repo_root/build/Release/inox"
elif [[ -x "$repo_root/build/$configuration/inox" ]]; then
    compiler_path="$repo_root/build/$configuration/inox"
elif [[ -x "$repo_root/build/inox" ]]; then
    compiler_path="$repo_root/build/inox"
fi

if [[ ! -x "$compiler_path" ]]; then
    echo "Compiler executable not found. Build the compiler first." >&2
    exit 1
fi

dist_root="$repo_root/dist"
package_root="$dist_root/$package_name"
zip_path="$dist_root/$package_name.zip"

rm -rf "$package_root" "$zip_path"
mkdir -p "$package_root/bin" "$package_root/stdlib" "$package_root/examples" "$package_root/licenses" "$package_root/docs"

cp "$compiler_path" "$package_root/bin/inox"
cp -R "$repo_root/stdlib/." "$package_root/stdlib/"
cp -R "$repo_root/examples/." "$package_root/examples/"
cp -R "$repo_root/licenses/." "$package_root/licenses/"
cp "$repo_root/docs/release/prebuilt-usage.md" "$package_root/docs/prebuilt-usage.md"
cp "$repo_root/docs/release/README.md" "$package_root/README.md"

for legal_file in LICENSE LICENSE.md NOTICE.md AUTHORS.md TRADEMARK.md; do
    [[ -f "$repo_root/$legal_file" ]] && cp "$repo_root/$legal_file" "$package_root/$legal_file"
done

(
    cd "$dist_root"
    zip -qr "$package_name.zip" "$package_name"
)

echo "Created release package: $zip_path"
echo "Publish it as a GitHub Release asset, for example:"
echo "https://github.com/<OWNER>/Inox/releases/latest/download/$package_name.zip"
