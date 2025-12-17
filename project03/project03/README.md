# Project 03: Heap Management

This is [Project 03] of [CSE.30341.FA25].

## Students

1. Leonardo Molina (lmolina3@nd.edu)
2. Sam Neisewander (sneisewa@nd.edu)

## Video

[Reflection Video](https://www.youtube.com/watch?v=cqPiF2FUEi8)

## Brainstorming

The following are questions that should help you in thinking about how to
approach implementing [Project 03].  For this project, responses to these
brainstorming questions are not required.

### Block

1. When **releasing** a `Block`:

    1. How do you determine if the `Block` you are releasing is at the end of
       the heap?

    2. How do you compute how much was actually allocated for the `Block`
       (including the header)?

    3. How do you adjust the heap pointer?

2. When **detaching** a `Block`, what pointers need to be updated?

3. When **merging** two `Block`s:

    1. How do you determine the **end** of the destination `Block`?

    2. How do you determine the **start** of the source `Block`?

    3. What is the **capacity** of the **merged** `Block`?

    4. What pointers need to be updated in the **merged** `Block`?

4. When **splitting** a `Block`:

    1. What do you do if the `Block`'s **capacity** is not large enough for a
       new `Block` header and the **aligned** `size`?

    2. What is the address of the new `Block` you split off?

    3. What is the capacity of the new `Block` you split off?

    4. What are the pointers of the new `Block`?

    5. What must be updated for the original `Block` (after the split)?

### Counters

1. What do we need to keep track of to compute the **internal fragmentation**
   of the **Free List**?

2. What do we need to keep track of to compute the **external fragmentation**
   of the **Free List**?

### Free List

1. What are we looking for in the **First Fit** algorithm?

2. What are we looking for in the **Best Fit** algorithm?

3. What are we looking for in the **Worst Fit** algorithm?

4. When we **insert** into the **Free List**:

    1. How do we maintain the sorted order of the **Free List**?

    2. After inserting the `Block` into the **Free List**, who should we
       attempt to **merge** with?

### POSIX

1. During a `malloc`, when do we **split** a `Block`, **allocate** a `Block`,
   and `detach` a `Block`?

2. When we `free`, when do we try to **release** a `Block` and what happens if
   that fails?

3. What happens during a `calloc`?

4. What happens during a `realloc`?

## Errata

> Describe any known errors, bugs, or deviations from the requirements.

## Acknowledgments

> List anyone you collaborated with or received help from (including TAs, other
students, and AI tools)

[Project 03]:       https://pnutz.h4x0r.space/courses/cse.30341.fa25/project03.html
[CSE.30341.FA25]:   https://pnutz.h4x0r.space/courses/cse.30341.fa25/
