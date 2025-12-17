# Project 01: Process Queue Shell

This is [Project 01] of [CSE.30341.FA25].

## Students

- Sam Neisewander (sneisewa@nd.edu)
- Leo Molina (lmolina3@nd.edu)

## Video

[Reflection Video](https://youtu.be/vCGOfKWL2V4)

## Brainstorming

The following are questions that should help you in thinking about how to
approach implementing [Project 01].

### Process

1. Given a command string such as `sleep 10`, how would you start a process
   that executes this command?  In particular, how would you handle the name of
   the program (ie. `sleep`) versus the arguments to the program (ie. `10`)?

   > We would first determine the number of arguments in the command by walking the string and counting the number of whitespace delimited substrings. Then, we would allocate an appropriately sized array of string pointers. Next, we would use `strtok` on the string to collect pointers to each argument in the string. Then, we would pass the array to `execvp` and use the substring at index 0 as the program name.

2. What signals do you need to send to a process to **pause** it (ie. suspend
   its execution) or to **resume** it (ie. continue its execution)?

   > `SIGSTOP` and `SIGCONT`

### Queue

1. What must happen when **pushing** a process to a `Queue`?

   >  - `process->next` gets null
      - `tail->next` gets the new process pointer
      - `tail` gets new process pointer
      - `queue->size` gets updated

2. What must happen when **popping** a process from a `Queue`?
   >  - save `head->next`
      - pop head
      - head gets saved `head-next`
      - `queue->size` gets updated

3. What must happen when **removing** a process from a `Queue`?
   >  - walk the list, looking ahead until find process
      - save `curr->next`
      - `curr->next` gets `curr->next->next`
    

### Shell

1. What **system call** will allow us to trigger a timer event at periodic
   intervals?  What signal does it send when a timer is triggered?

   > `alarm`, which sends the `SIGALRM`

2. How will you interweave reading **input** from the user, handling periodic
   **timers**, and waiting for **child** processes?

   > 1. Register a signal handler for the `SIGALRM` signal
   > 2. Register a signal handler for the `SIGCHLD` signal
   > 3. Set an alarm for `s->timeout` microseconds
   > 4. Enter `while` loop that awaits user input using `fgets`
   > 5. When a child terminates, update the process queue with its new status
   > 6. Wait for children that have terminated based on process struct status. Update the process queue based on the chosen scheduling policy.
   > 7. Repeat steps 2-6 until user sends `quit` to `stdin`

3. What functions would allow you to parse strings with various arguments?

   > `strtok`, `atoi`

### Scheduler

1. What needs to be created when calling `scheduler_add` and where should this
   object go?

   > A new process struct should be created, which involves forking a new child and immediately stopping it. Then you would put its `pid` and `state` into a new process struct, which would then get added to the process queue.

2. How would you determine if you should display a particular queue in the
   `scheduler_status` function?

   > If the user runs the `status` command in `pqsh` without an argument, everying gets displayed. Otherwise, if the user passes `running | waiting | finished`, display only the appropriate queue. 

3. How would you wait for a process without blocking? What information do you
   need to update when a process terminates?

   > `wait` returns if a child process undergoes any state change, so if we stop all of the children with `SIGSTOP` before running `waitpid`, we will always immediately get a return value with the current state of the child. When a process terminates, we need to update its **process struct** and move it to the **finished process queue**.

4. What should the scheduler handler functions do when they receive either a
   `SIGALRM` or a `SIGCHLD`?

   > `SIGALRM`: call `scheduler_wait` to handle terminated children
      `SIGCHLD`: Update the child's status in the process queue struct.

5. When and where are `scheduler_wait` and `scheduler_next` called?

   >  `scheduler_wait`: In the `SIGALRM` handler.
      `scheduler_next`: In the `SIGALRM` handler.

### Timestamp

1. How would you use the result of `clock_gettime` to return a `double`
   representing the current time in **seconds**?

## Errata

> Describe any known errors, bugs, or deviations from the requirements.

If pqsh runs a process that prints to `stdout`, the process will write on top of the psqh prompt instead of pushing it to a new line.

## Acknowledgments

> List anyone you collaborated with or received help from (including TAs, other
students, and AI tools)

- Professor Bui
- Henry Jochaniewicz

[Project 01]: https://pnutz.h4x0r.space/courses/cse.30341.fa25/project01.html
[CSE.30341.FA25]: https://pnutz.h4x0r.space/courses/cse.30341.fa25/
