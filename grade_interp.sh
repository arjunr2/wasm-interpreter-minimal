#!/bin/bash

if [ $# = 0 ]; then
    WEERUN='weerun'
else
    WEERUN=$1
    shift
fi

T=${OUT:=/tmp/$USER/cs17670/$STUDENT/proj2-test/}
mkdir -p $T


if [ ! -x "$WEERUN" ]; then
    printf "##+weerun\n"
    printf "##-fail, weerun not found: %s\n" $WEERUN
    exit 1
fi

if [ $# = 0 ]; then
    TESTS=$(ls proj2-tests/*.wee.wasm)
else
    TESTS=$@
fi

# substitute for the "timeout" command on MacOS
function xtimeout {
  timeout=$1
  shift
  command=("$@")
  (
    "${command[@]}" &
    runnerpid=$!
    trap -- '' SIGTERM
    ( # killer job
      sleep $timeout
      if ps -p $runnerpid > /dev/null; then
	  echo "Timed out, killing"
          kill -SIGKILL $runnerpid 2>/dev/null
      fi
    ) &
    killerpid=$!
    wait $runnerpid
    kill -SIGKILL $killerpid
  )
}

function run_one() {
    echo $WEERUN $f $ARGS
    xtimeout 5s $WEERUN $f $ARGS > $OUT 2>$ERR
    # ignore case
    diff -i $EXP $OUT > $DIFF
    OK=$?
    if [ ! -s $DIFF ]; then
	# remove empty diffs
	rm $DIFF
    else
	cat $DIFF
    fi
    
    if [ $OK = 0 ]; then
	return 0
    else
	echo diff $EXP $OUT
	printf "##-fail: $ARGS\n"
	return 1
    fi

}

for f in $TESTS; do
    # replace slashes with _
    t=${f//\//_}
    # replace .wasm with .runs
    runs=${f/%.wee.wasm/.runs}
    
    printf "##+%s\n" $t

    OUT=$T/$t.out
    ERR=$T/$t.err
    EXP=$T/$t.expect
    DIFF=$T/$t.diff

    if [ -f $runs ]; then
	# do multiple runs for this test
	ok=1
	while IFS= read -r line
	do
	    ARGS=$(echo $line | cut -d= -f1)
	    EXPECT=$(echo $line | cut -d= -f2)
	    echo $EXPECT > $EXP

	    run_one

	    if [ $? != 0 ]; then
		ok=0
		break;
	    fi
	done < $runs
	if [ $ok = 1 ]; then
	    printf "##-ok\n"
	fi
    else
	# do one run
	e=${f/%.wee.wasm/.expect}
	args=${f/%.wee.wasm/.args}
	cp $e $EXP
	
	if [ -f $args ]; then
	    ARGS=$(cat $args)
	else
	    ARGS=""
	fi

	run_one

	if [ $? = 0 ]; then
	    printf "##-ok\n"
	fi
    fi
done
