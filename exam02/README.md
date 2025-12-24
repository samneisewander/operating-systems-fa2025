CSE 30341: Operating System Principles 

# Exam 02 - Redemption

## Concurrent Curl (4 Points)

For extra credit, you may implement a C program similar to miles.py from Homework 06 in Systems Programming (Spring 2024): a program that downloads multiple URLs concurrently.

Instead of a process pool, you are to use threads and the synchronization mechanisms of your choice (ie. locks, condition variables, semaphores).

Instead of using regular expressions to extract URLs from the initial URL (command line argument), all the URLs will be given as command line arguments:

```Usage: program [-d DESTINATION] [-n CONCURRENCY] URLs…```

- The `DESTINATION` is the directory where you should store the downloaded 
  files (default is the current directory).
  
If the `DESTINATION` does not exist, then you should create it.  

Files should be stored as `$DESTINATION/$(basename $URL)`.
- The `CONCURRENCY` is the maximum number of concurrent downloads (default is 2).

To manage the concurrency, you should use one of the concurrency patterns from Reading 05 or Reading 06.

To perform the HTTP requests, you are to use `libcurl` like you did in Project 02.

To aid in debugging and tracing, print "Starting $URL…" before performing the libcurl operations and "Finished $URL: $STATUS" after the operations are completed.

- If any of the URLs fail to download the program should exit with failure after all URLs have been processed. Otherwise, it should exit with success. 

Here is an example of the program in action:

```bash
$ ./program -d html -n 2 https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html https://pnutz.h4x0r.space/archive/cse.20289.sp24/homework03.html https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html 
Starting https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html...
Starting https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html...
Finished https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework02.html: 0
Starting https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html...
Finished https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework01.html: 0
Starting https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework03.html...
Finished https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework04.html: 0
Finished https://pnutz.h4x0r.space/archive/teaching/cse.20289.sp24/homework03.html: 0

$ sha1sum html/*
a7304b1620c764c5860b02f1a7ae00c027f59d6d  html/homework01.html
cd7b3427128d2cb21c721086f32bb122f7aa94ef  html/homework02.html
0306fab5603404380af624a5f3ac46203339e490  html/homework03.html
8a3142f7b2928ad3a0cdf7b50474f1de39513d31  html/homework04.html
```


To get started, you are to switch the master branch in your assignments repository and apply the following patch: [https://yld.me/raw/iKux] 

```bash
	$ git switch master
	$ curl -sL https://yld.me/raw/iKux | git am -
```

This will provide you with a Makefile, program.c, and thread.h.  Afterwards, create an exam02 branch, modify these files as necessary, and commit your work to that branch.

You can use make to verify your program works

```bash
$ make
Testing exam02 program...
 program                                                      ... Success
 program [1]                                                  ... Success
 program [1] (strace)                                         ... Success
 program [2]                                                  ... Success
 program [2] (strace)                                         ... Success
 program [8]                                                  ... Success
 program [8] (strace)                                         ... Success
 program -d /tmp/exam02.1000/html [8]                         ... Success
 program -d /tmp/exam02.1000/html [8] (strace)                ... Success
 program -d /tmp/exam02.1000/html -n 4 [8]                    ... Success
 program -d /tmp/exam02.1000/html -n 4 [8] (strace)           ... Success
   Score 3.00 / 3.00
  Status Success
```

You have until midnight, Tuesday, November 4, 2025 to submit this redemption by making a Pull Request and assigning it to the instructor.

In your Pull Request, include a reflection video (no more than 5 minutes in length), that discusses the following:

- How does the program work (ie. overall architecture)?
- Which synchronization primitives did you use and why?
- Which concurrency patterns did you use and why?
- Is your program task or data parallel (or both)?

Use this program to download several large files concurrently (say the Linux Kernel).  Does having a concurrent HTTP client improve your download speed?  Explain.

Whether or not you used AI tools (and how you used them).

You will be graded holistically based on your code (correctness, style) and reflection (correctness, succinctness, integrity).