#!/bin/bash

FUNCTIONAL=test_queue
WORKSPACE=/tmp/$FUNCTIONAL.$(id -u)
FAILURES=0

error() {
    echo "$@"
    [ -r $WORKSPACE/test ] && (echo; cat $WORKSPACE/test; echo)
    FAILURES=$((FAILURES + 1))
}

cleanup() {
    STATUS=${1:-$FAILURES}
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

ARGUMENTS="1 1 1"
printf " %-60s ... " "$(printf "producers: %4d, consumers: %4d, messages: %4d" $ARGUMENTS)"
valgrind --leak-check=full bin/$FUNCTIONAL $ARGUMENTS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test) -ne 0 ]; then
    error "Failure (Valgrind)"
else
    echo "Success"
fi

ARGUMENTS="2 1 128"
printf " %-60s ... " "$(printf "producers: %4d, consumers: %4d, messages: %4d" $ARGUMENTS)"
valgrind --leak-check=full bin/$FUNCTIONAL $ARGUMENTS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test) -ne 0 ]; then
    error "Failure (Valgrind)"
else
    echo "Success"
fi

ARGUMENTS="4 2 1024"
printf " %-60s ... " "$(printf "producers: %4d, consumers: %4d, messages: %4d" $ARGUMENTS)"
valgrind --leak-check=full bin/$FUNCTIONAL $ARGUMENTS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test) -ne 0 ]; then
    error "Failure (Valgrind)"
else
    echo "Success"
fi

echo
