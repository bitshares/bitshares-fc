#!/bin/sh

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <boost-unit-test-executable> [arguments]" 1>&2
    exit 1
fi

CACHE="$( find . -name CMakeCache.txt )"
if [ "$CACHE" != "" ]; then
    BOOST_INC="$( grep Boost_INCLUDE_DIR:PATH "$CACHE" | cut -d= -f 2 )"
    if [ "$BOOST_INC" != "" ]; then
	BOOST_VERSION="$( grep '^#define *BOOST_VERSION ' "$BOOST_INC/boost/version.hpp" | sed 's=^.* ==' )"
    fi
fi

if [ "$BOOST_VERSION" = "" -o "$BOOST_VERSION" -lt 105900 ]; then
    echo "Boost version '$BOOST_VERSION' - executing tests serially"
    "$@"
elif ! command -v parallel >/dev/null 2>&1; then
    echo "Can not find the 'parallel' command - executing tests serially"
    "$@"
else
  echo "=== $1 list_content test begin ==="
  "$1" --list_content 2>&1 \
    | grep '\*$' \
    | sed 's=\*$==;s=^    =/='
  echo "=== $1 list_content test end ==="
  "$1" --list_content 2>&1 \
    | grep '\*$' \
    | sed 's=\*$==;s=^    =/=' \
    | (while read t; do
        case "$t" in
        /*) echo "$pre$t"; found=1; ;;
        *) if [ -n "$pre" -a "$found" = "0" ]; then
              echo "$pre"
           fi
           pre="$t"
           found=0
           ;;
        esac
      done
      if [ -n "$pre" -a "$found" = "0" ]; then
        echo "$pre"
      fi) \
    | parallel --tag --halt soon,fail=1 "$@" -t {}
fi
