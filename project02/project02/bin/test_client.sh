#!/bin/bash

FUNCTIONAL=test_client
WORKSPACE=/tmp/$FUNCTIONAL.$(id -u)
FAILURES=0

error() {
    echo "$@"
    [ -r $WORKSPACE/test ] && (echo; cat $WORKSPACE/test; echo)
    FAILURES=$((FAILURES + 1))
}

find_port() {
    for port in $(seq 9000 9999); do
    	if ! ss -H4tlpn | awk '{print $4}' | cut -d : -f 2 | grep -q $port; then
    	    echo $port
    	    break
	fi
    done
}

cleanup() {
    STATUS=${1:-$FAILURES}
    kill $SERVERPID
    rm -fr $WORKSPACE
    exit $STATUS
}

mkdir $WORKSPACE

trap "cleanup" EXIT
trap "cleanup 1" INT TERM

echo "Testing $FUNCTIONAL ..."

if [ ! -x bin/$FUNCTIONAL ]; then
    echo "Failure: bin/$FUNCTIONAL is not executable!"
    exit 1
fi

if [ ! -x bin/mq_server.py ]; then
    echo "Failure: bin/mq_server.py is not executable!"
    exit 2
fi

PORT=$(find_port)

./bin/mq_server.py --port=$PORT > /dev/null 2>&1 &
SERVERPID=$!

printf " %-60s ... " "port: $PORT"
valgrind --leak-check=full bin/$FUNCTIONAL localhost $PORT &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test) -ne 0 ]; then
    error "Failure (Valgrind)"
else
    echo "Success"
fi

echo
