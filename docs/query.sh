#!/usr/bin/env bash
# Usage: bash docs/query.sh <AREA|TNN|ID-fragment> [TNN]
# Examples:
#   bash docs/query.sh ENV
#   bash docs/query.sh HAL T10
#   bash docs/query.sh TB371FC-T10
set -u
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
q1="${1:-}"; q2="${2:-}"
if [ -z "$q1" ]; then echo "usage: bash docs/query.sh <AREA|TNN|ID> [TNN]" >&2; exit 2; fi
if [ -n "$q2" ]; then
  grep -rh -- "ID:TB371FC-${q2}-.*-${q1}-" "$ROOT"/*.md 2>/dev/null || grep -rh -- "$q1" "$ROOT"/*.md | grep -- "$q2" || exit 1
else
  case "$q1" in
    ENV|VER|TOUCH|PANEL|HAL|DTB|BUILD|PROC|META) grep -rh -- "ID:TB371FC-T.*-${q1}-" "$ROOT"/*.md 2>/dev/null || exit 1 ;;
    *) grep -rh -- "$q1" "$ROOT"/*.md 2>/dev/null || exit 1 ;;
  esac
fi
