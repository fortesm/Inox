#!/usr/bin/env bash
# SPDX-License-Identifier: MPL-2.0
# Copyright © 2026 Marcelo Fortes and Inox contributors. All rights reserved.

set -euo pipefail

configuration="${1:-Release}"
package_name="${2:-inox-linux-x64}"
explicit_compiler_path="${3:-}"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

compiler_path=""
if [[ -n "$explicit_compiler_path" ]]; then
    compiler_path="$explicit_compiler_path"
elif [[ "$configuration" == "Release" && -x "$repo_root/build-linux/inox" ]]; then
    compiler_path="$repo_root/build-linux/inox"
elif [[ "$configuration" == "Release" && -x "$repo_root/build/Release/inox" ]]; then
    compiler_path="$repo_root/build/Release/inox"
elif [[ -x "$repo_root/build/$configuration/inox" ]]; then
    compiler_path="$repo_root/build/$configuration/inox"
elif [[ -x "$repo_root/build/inox" ]]; then
    compiler_path="$repo_root/build/inox"
elif [[ -x "$repo_root/build-linux/inox" ]]; then
    compiler_path="$repo_root/build-linux/inox"
fi

if [[ ! -x "$compiler_path" ]]; then
    echo "Compiler executable not found. Build the compiler first." >&2
    echo "Example: cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ && cmake --build build-linux" >&2
    exit 1
fi

dist_root="$repo_root/dist"
package_root="$dist_root/$package_name"
zip_path="$dist_root/$package_name.zip"

rm -rf "$package_root" "$zip_path"
mkdir -p "$package_root/bin" "$package_root/stdlib" "$package_root/examples" "$package_root/licenses" "$package_root/output" "$package_root/docs"

cp "$compiler_path" "$package_root/bin/inox"
cp -R "$repo_root/stdlib/." "$package_root/stdlib/"
cp -R "$repo_root/examples/." "$package_root/examples/"
# libinoxrt (exceptions, Strings, Files) is found beside the compiler.
runtime_path="$(dirname "$compiler_path")/libinoxrt.a"
if [[ ! -f "$runtime_path" ]]; then
    echo "libinoxrt.a not found beside the compiler: $runtime_path" >&2
    exit 1
fi
cp "$runtime_path" "$package_root/bin/libinoxrt.a"
# The manual and the constitution (the language law).
cp "$repo_root/docs/site/index.html" "$package_root/docs/index.html"
mkdir -p "$package_root/docs/constitution"
for law_file in spec.md engineering.md revisions.md; do
    cp "$repo_root/openspec/specs/constitution/$law_file" "$package_root/docs/constitution/$law_file"
done
[[ -d "$repo_root/licenses" ]] && cp -R "$repo_root/licenses/." "$package_root/licenses/"

for legal_file in LICENSE LICENSE.md NOTICE.md AUTHORS.md TRADEMARK.md; do
    [[ -f "$repo_root/$legal_file" ]] && cp "$repo_root/$legal_file" "$package_root/$legal_file"
done

if [[ -f "$repo_root/docs/release/README.md" ]]; then
    cp "$repo_root/docs/release/README.md" "$package_root/README.md"
elif [[ -f "$repo_root/README.md" ]]; then
    cp "$repo_root/README.md" "$package_root/README.md"
else
    echo "# Inox Linux x64 Release" > "$package_root/README.md"
fi

cat > "$package_root/set-inox-env.sh" <<'EOS'
#!/usr/bin/env bash
release_root="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PATH="$release_root/bin:$PATH"
export INOX_STDLIB="$release_root/stdlib"
export INOX_OUTPUT_DIR="$release_root/output"
echo "Inox release environment configured."
echo "INOX_STDLIB=$INOX_STDLIB"
echo "INOX_OUTPUT_DIR=$INOX_OUTPUT_DIR"
echo "Manual: $release_root/docs/index.html"
echo "Language law: $release_root/docs/constitution/spec.md"
command -v inox || true
EOS
chmod +x "$package_root/set-inox-env.sh" "$package_root/bin/inox"

echo "This directory is the default output location for native programs generated from Inox examples when INOX_OUTPUT_DIR points here." > "$package_root/output/README.txt"

(
    cd "$dist_root"
    zip -qr "$package_name.zip" "$package_name"
)

echo "Created release package: $zip_path"
echo "Publish it as a GitHub Release asset:"
echo "https://github.com/fortesm/Inox/releases/latest/download/$package_name.zip"
