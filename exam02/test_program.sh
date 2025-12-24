#!/bin/bash

WORKSPACE=/tmp/exam02.$(id -u)
FAILURES=0
POINTS=3
DESTINATION="."
CONCURRENCY=2

case $(uname) in
Darwin)
    SHA1SUM=shasum
    ;;
*)
    SHA1SUM=sha1sum
    ;;
esac

error() {
    echo "$@"
    [ -r $WORKSPACE/test ] && (echo; cat $WORKSPACE/test; echo)
    FAILURES=$((FAILURES + 1))
}

cleanup() {
    STATUS=${1:-$FAILURES}
    rm -fr $WORKSPACE
    rm -f homework*.html

    if [ "$STATUS" -eq 0 ]; then
        echo "  Status Success"
    else
        echo "  Status Failure"
    fi

    echo
    exit $STATUS
}

grep_all() {
    for pattern in $1; do
	if ! grep -q -E "$pattern" $2; then
	    echo "FAILURE: Missing '$pattern' in '$2'" > $WORKSPACE/test
	    return 1;
	fi
    done
    return 0;
}

grep_any() {
    for pattern in $1; do
	if grep -q -E "$pattern" $2; then
	    echo "FAILURE: Contains '$pattern' in '$2'" > $WORKSPACE/test
	    return 1;
	fi
    done
    return 0;
}

sha1sums() {
    cat <<EOF
a7304b1620c764c5860b02f1a7ae00c027f59d6d  homework01.html
cd7b3427128d2cb21c721086f32bb122f7aa94ef  homework02.html
0306fab5603404380af624a5f3ac46203339e490  homework03.html
8a3142f7b2928ad3a0cdf7b50474f1de39513d31  homework04.html
b72cc8878c9847f99644c2c926f4589ea18d691d  homework05.html
4e12047feb3e57f5803ca8c88717468c44e92564  homework06.html
7303f76e45888014e693ad50b968101bb1776492  homework07.html
94291a32e94faa1c57b5c09c8a00df7c3bb9d92b  homework08.html
97c29f3513c30fbf9a54980d3c148ed2689588c1  homework09.html
dbec065f7de2c44056d4596190abd03e801d71aa  homework10.html
EOF
}

check_sha1sums() {
    for url in $URLS; do
	name=$(basename $url)
	if [ "$name" = "homeworkXX.html" ]; then
	    continue
	fi

	if [ "$($SHA1SUM $DESTINATION/$name | awk '{print $1}')" != "$(sha1sums | grep $name | awk '{print $1}')" ]; then
	    return 1
	fi
    done
    return 0
}

check_concurrency() {
    python3 <<EOF
import sys
active=0
for line in open('$WORKSPACE/test'):
    if line.startswith('Start'):
        active += 1
    elif line.startswith('Finish'):
        active -= 1

    if active > $CONCURRENCY:
        sys.exit(1)

sys.exit(0)
EOF
}

mkdir $WORKSPACE

trap "cleanup" EXIT
trap "cleanup 1" INT TERM

echo "Testing exam02 program..."


printf " %-60s ... " "program"
valgrind --leak-check=full ./program &> $WORKSPACE/test
if [ $? -ne 1 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
else
    echo "Success"
fi


URLS="https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework06.html"
printf " %-60s ... " "program [$(echo $URLS | wc -w)]"
valgrind --leak-check=full ./program $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
elif ! check_sha1sums; then
    error "Failure (Contents)"
elif ! check_concurrency; then
    error "Failure (Concurrency)"
else
    echo "Success"
fi

printf " %-60s ... " "program [$(echo $URLS | wc -w)] (strace)"
strace -e clone,clone3 ./program $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $(echo $URLS | wc -w) -a $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $CONCURRENCY ]; then
    error "Failure (Strace)"
else
    echo "Success"
fi

URLS="https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework06.html
https://fake.asdf/homeworkXX.html"
printf " %-60s ... " "program [$(echo $URLS | wc -w)]"
valgrind --leak-check=full ./program $URLS &> $WORKSPACE/test
if [ $? -eq 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
elif ! check_sha1sums; then
    error "Failure (Contents)"
elif ! check_concurrency; then
    error "Failure (Concurrency)"
else
    echo "Success"
fi

printf " %-60s ... " "program [$(echo $URLS | wc -w)] (strace)"
strace -e clone,clone3 ./program $URLS &> $WORKSPACE/test
if [ $? -eq 0 ]; then
    error "Failure (Exit Code)"
elif [ $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $(echo $URLS | wc -w) -a $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $CONCURRENCY ]; then
    error "Failure (Strace)"
else
    echo "Success"
fi

URLS="https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework03.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework05.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework06.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework07.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework08.html"
printf " %-60s ... " "program [$(echo $URLS | wc -w)]"
valgrind --leak-check=full ./program $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
elif ! check_sha1sums; then
    error "Failure (Contents)"
elif ! check_concurrency; then
    error "Failure (Concurrency)"
else
    echo "Success"
fi

printf " %-60s ... " "program [$(echo $URLS | wc -w)] (strace)"
strace -e clone,clone3 ./program $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $(echo $URLS | wc -w) -a $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $CONCURRENCY ]; then
    error "Failure (Strace)"
else
    echo "Success"
fi


URLS="https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework03.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework05.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework06.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework07.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework08.html"
DESTINATION=$WORKSPACE/html
printf " %-60s ... " "program -d $DESTINATION [$(echo $URLS | wc -w)]"
valgrind --leak-check=full ./program -d $DESTINATION $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
elif ! check_sha1sums; then
    error "Failure (Contents)"
elif ! check_concurrency; then
    error "Failure (Concurrency)"
else
    echo "Success"
fi

printf " %-60s ... " "program -d $DESTINATION [$(echo $URLS | wc -w)] (strace)"
strace -e clone,clone3 ./program -d $DESTINATION $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $(echo $URLS | wc -w) -a $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $CONCURRENCY ]; then
    error "Failure (Strace)"
else
    echo "Success"
fi


URLS="https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework03.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework05.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework06.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework07.html
https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework08.html"
DESTINATION=$WORKSPACE/html
CONCURRENCY=4
printf " %-60s ... " "program -d $DESTINATION -n $CONCURRENCY [$(echo $URLS | wc -w)]"
valgrind --leak-check=full ./program -d $DESTINATION -n $CONCURRENCY $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(awk '/ERROR SUMMARY:/ {print $4}' $WORKSPACE/test | sort -rn | tail -n 1) -ne 0 ]; then
    error "Failure (Valgrind)"
elif ! check_sha1sums; then
    error "Failure (Contents)"
elif ! check_concurrency; then
    error "Failure (Concurrency)"
else
    echo "Success"
fi

printf " %-60s ... " "program -d $DESTINATION -n $CONCURRENCY [$(echo $URLS | wc -w)] (strace)"
strace -e clone,clone3 ./program -d $DESTINATION -n $CONCURRENCY $URLS &> $WORKSPACE/test
if [ $? -ne 0 ]; then
    error "Failure (Exit Code)"
elif [ $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $(echo $URLS | wc -w) -a $(grep -v ENOSYS $WORKSPACE/test | grep -c CLONE_THREAD) -ne $CONCURRENCY ]; then
    error "Failure (Strace)"
else
    echo "Success"
fi


TESTS=$(($(grep -c Success $0) - 1))
SCORE=$(python3 <<EOF
print("{:0.2f} / $POINTS.00".format(($TESTS - $FAILURES) * $POINTS.00 / $TESTS))
EOF
)
echo "   Score $SCORE"
