#!/usr/bin/env bash
# Build Universal Game Spoof (UGSpoof)
#   ./build.sh          -> tes + build zip modul ke out/
#   ./build.sh test     -> hanya jalankan tes
#   ./build.sh lib      -> hanya compile library zygisk
# Env opsional:
#   ANDROID_NDK_HOME  pakai NDK resmi (kalau kosong: clang+lld sistem, mode freestanding)
#   AUTHOR            nama author di module.prop (default: UGSpoof contributors)
#   GITHUB_REPO       "user/repo" untuk updateJson Magisk (kosong = tanpa auto-update)
set -euo pipefail
cd "$(dirname "$0")"
ROOT=$PWD
VERSION=$(tr -d ' \n' < VERSION)
IFS=. read -r MA MI PA <<< "$VERSION"
VCODE=$(( MA * 10000 + MI * 100 + PA ))
AUTHOR=${AUTHOR:-UGSpoof contributors}
OUT=$ROOT/out; BUILD=$ROOT/build
mkdir -p "$OUT" "$BUILD"

CFLAGS="-O2 -fPIC -ffreestanding -fno-builtin -fno-stack-protector -fvisibility=hidden -Wall -Wextra -Wno-unused-parameter"
LDFLAGS="-nostdlib -shared -Wl,--hash-style=both -Wl,-z,max-page-size=16384 -Wl,--no-undefined -Wl,-z,noexecstack -Wl,--gc-sections -s"

build_lib() {
  if [ -n "${ANDROID_NDK_HOME:-}" ]; then
    TC="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64"
    CC="$TC/bin/aarch64-linux-android29-clang"
    LIBS="-L$TC/sysroot/usr/lib/aarch64-linux-android/29"
    echo "[*] Compile pakai NDK: $CC"
  else
    command -v clang >/dev/null || { echo "clang tidak ada (apt install clang lld)"; exit 1; }
    CC="clang --target=aarch64-linux-android29 -fuse-ld=lld"
    # Stub libc/libdl/liblog: cukup untuk linking; di HP yang dipakai library asli.
    S=$BUILD/stubs; mkdir -p "$S"
    echo 'void mmap(){} void mprotect(){} void open(){} void read(){} void write(){} void close(){} void getauxval(){}' > "$S/c.c"
    echo 'void dlopen(){} void dlsym(){} void dladdr(){}' > "$S/dl.c"
    echo 'void __android_log_print(){}' > "$S/log.c"
    for l in c dl log; do $CC -shared -nostdlib -Wl,-soname,lib$l.so -o "$S/lib$l.so" "$S/$l.c"; done
    LIBS="-L$S"
    echo "[*] Compile pakai clang sistem (freestanding)"
  fi
  # shellcheck disable=SC2086
  $CC $CFLAGS $LDFLAGS $LIBS -o "$BUILD/arm64-v8a.so" src/spoof.c -lc -ldl -llog
  echo "[+] $BUILD/arm64-v8a.so ($(stat -c %s "$BUILD/arm64-v8a.so") byte)"
}

run_tests() {
  HCC=$(command -v gcc || command -v clang)
  echo "== Tes relocator =="
  $HCC -O1 -w -o "$BUILD/test_reloc" tests/test_reloc.c && "$BUILD/test_reloc"
  echo "== Tes companion & config =="
  FX=$BUILD/fixture; rm -rf "$FX"; mkdir -p "$FX"
  cp -r module/profiles "$FX/"
  cat > "$FX/config.txt" <<'CFG'
profile=redmagic_9s_pro
packages=com.game.a, com.game.b
map.com.game.c=galaxy_s24_ultra
map.com.evil=../../x
enable_vulkan=0
prop.ro.mediatek.*=
CFG
  $HCC -O1 -w -DMODDIR="\"$FX\"" -o "$BUILD/test_companion" tests/test_companion.c -ldl && "$BUILD/test_companion"
}

package() {
  STAGE=$BUILD/stage; rm -rf "$STAGE"; mkdir -p "$STAGE/zygisk"
  cp -r module/. "$STAGE/"; rm -f "$STAGE/module.prop.in"
  cp "$BUILD/arm64-v8a.so" "$STAGE/zygisk/"
  UJ=""; [ -n "${GITHUB_REPO:-}" ] && UJ="https://raw.githubusercontent.com/$GITHUB_REPO/main/update.json"
  sed -e "s|@VERSION@|v$VERSION|" -e "s|@VERSIONCODE@|$VCODE|" -e "s|@AUTHOR@|$AUTHOR|" -e "s|@UPDATEJSON@|$UJ|" \
      module/module.prop.in > "$STAGE/module.prop"
  [ -z "$UJ" ] && sed -i '/^updateJson=$/d' "$STAGE/module.prop"
  ZIP="$OUT/UGSpoof-v$VERSION.zip"; rm -f "$ZIP"
  (cd "$STAGE" && zip -qr9 "$ZIP" .)
  echo "[+] $ZIP"
  if [ -n "${GITHUB_REPO:-}" ]; then
    cat > "$OUT/update.json" <<JSON
{
  "version": "v$VERSION",
  "versionCode": $VCODE,
  "zipUrl": "https://github.com/$GITHUB_REPO/releases/download/v$VERSION/UGSpoof-v$VERSION.zip",
  "changelog": "https://raw.githubusercontent.com/$GITHUB_REPO/main/CHANGELOG.md"
}
JSON
    echo "[+] $OUT/update.json (commit file ini ke root repo)"
  fi
}

case "${1:-all}" in
  test) run_tests ;;
  lib)  build_lib ;;
  all)  run_tests; build_lib; package ;;
  *) echo "pakai: $0 [all|test|lib]"; exit 1 ;;
esac
