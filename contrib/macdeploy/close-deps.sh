#!/bin/bash
# close-deps.sh — recursively close the transitive dependency closure of a
# Dragonchain-Qt.app bundle, then HARD-FAIL if any load-command still resolves
# to a file that does not exist inside the bundle.
#
# Why this exists: contrib/macdeploy/macdeployqtplus rewrites @rpath and
# absolute-path install names, but Homebrew's boost bottles reference their
# sibling libraries via @loader_path/libboost_*.dylib. macdeployqtplus does not
# copy those second-level siblings, so the app ships libboost_filesystem/thread/
# program_options but NOT libboost_atomic/container/date_time. dyld then aborts
# at launch ("Library not loaded: @loader_path/libboost_atomic.dylib") on every
# macOS. This script copies the missing siblings in from Homebrew and enforces a
# fully-closed closure in CI.
#
# Usage: close-deps.sh dist/Dragonchain-Qt.app
set -uo pipefail

APP="${1:?usage: close-deps.sh <path/to/Dragonchain-Qt.app>}"
FW="$APP/Contents/Frameworks"

# ---- candidate locations a single load-command may resolve to ----
# Prints absolute paths; prints nothing for system libraries (always present).
candidates() {
  local bin="$1" dep="$2"
  local bindir; bindir="$(dirname "$bin")"
  case "$dep" in
    /usr/lib/*|/System/Library/*)
      return 0 ;;
    @executable_path/*)
      printf '%s\n' "$APP/Contents/MacOS/${dep#@executable_path/}" ;;
    @loader_path/*)
      printf '%s\n' "$bindir/${dep#@loader_path/}" ;;
    @rpath/*)
      # flattened bundle: try the referrer's dir, Frameworks, and MacOS
      local b="${dep#@rpath/}"
      printf '%s\n' "$bindir/$b" "$FW/$b" "$APP/Contents/MacOS/$b" ;;
    /*)
      # leftover absolute (Homebrew) path — expected to have been rewritten to
      # Frameworks/<basename>; flag it if that file is not present.
      printf '%s\n' "$FW/$(basename "$dep")" ;;
  esac
}

# Print "basename|dep|referrer" for every load-command that resolves to no file.
collect_unresolved() {
  local f dep c found
  find "$APP/Contents/MacOS" "$APP/Contents/Frameworks" "$APP/Contents/PlugIns" \
       -type f 2>/dev/null | while read -r f; do
    file "$f" 2>/dev/null | grep -q 'Mach-O' || continue
    while read -r dep; do
      [ -z "$dep" ] && continue
      found=""
      while read -r c; do
        [ -z "$c" ] && continue
        if [ -e "$c" ]; then found="$c"; break; fi
      done < <(candidates "$f" "$dep")
      [ -z "$found" ] && printf '%s|%s|%s\n' "$(basename "$dep")" "$dep" "$f"
    done < <(otool -L "$f" 2>/dev/null | tail -n +2 | awk '{print $1}')
  done
}

echo "=== close-deps.sh: closing dependency closure for $APP ==="

round=0
while [ $round -lt 8 ]; do
  round=$((round + 1))
  needs="$(collect_unresolved)"
  [ -z "$needs" ] && { echo "依赖闭包在第 $((round-1)) 轮达成"; break; }

  added=0
  while IFS='|' read -r base dep ref; do
    [ -z "$base" ] && continue
    src="$(find /opt/homebrew/opt/*/lib /opt/homebrew/lib \
                  /usr/local/opt/*/lib /usr/local/lib \
            -name "$base" -type f 2>/dev/null | head -1)"
    if [ -z "$src" ]; then
      echo "  [warn] 找不到 $base 的来源（referenced by ${ref#$APP/}）" >&2
      continue
    fi
    cp "$src" "$FW/$base"
    chmod 755 "$FW/$base"
    install_name_tool -id "@executable_path/../Frameworks/$base" "$FW/$base" 2>/dev/null || true
    for d in $(otool -L "$FW/$base" 2>/dev/null | tail -n +2 | awk '{print $1}'); do
      case "$d" in
        @loader_path/*|@rpath/*|/opt/homebrew/*|/usr/local/*)
          db="$(basename "$d")"
          [ "$d" = "@executable_path/../Frameworks/$db" ] && continue
          install_name_tool -change "$d" "@executable_path/../Frameworks/$db" "$FW/$base" 2>/dev/null || true
          ;;
      esac
    done
    echo "  [+] 补齐 $base"
    added=$((added + 1))
  done <<< "$needs"
  [ $added -eq 0 ] && { echo "本轮无进展，停止（存在找不到来源的库）"; break; }
done

echo "=== 依赖闭包终检（必须为空）==="
final="$(collect_unresolved)"
if [ -n "$final" ]; then
  while IFS='|' read -r base dep ref; do
    [ -z "$base" ] && continue
    echo "UNRESOLVED: $dep  (referenced by ${ref#$APP/})"
  done <<< "$final"
  echo "::error::依赖闭包未完成，禁止发布"
  exit 1
fi
echo "依赖闭包验证通过：所有 load-command 均解析到包内存在的文件"
