#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

PROJ2=$(cd $DIR/.. && pwd)
WEEIFY=$PROJ2/weeify

WAT2WASM=$(which wat2wasm)

if [[ ! -x $WAT2WASM ]]; then
   echo Error: wat2asm not found in PATH
   exit 1
fi

if [[ ! -x $WEEIFY ]]; then
   echo Error: weeify not found $DIR
   exit 1
fi

if [ $# = 0 ]; then
    FILES="*.wat"
else
    FILES="$@"
fi

for f in $FILES; do
    wat2wasm $f
    $WEEIFY -o ${f/%.wat/.wee.wasm} ${f/%.wat/.wasm}
done
