# Git magic

## Fixing the borked branch

[Learn git](https://www.atlassian.com/git)
Called a commit hash: `badf0e6 fixed duplicate messages`

```bash
git log --pretty=oneline --abbrev-commit    # Print commit history one line per commit
git rebase -i                               # Modify local commit histroy
git checkout dev src/queue.c                # Checkout a specific version of a file
apt list --installed                        # Print all installed packages
```

# Using ncurses

Documentation that Leo says "isn't ass": [Docs](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/index.html)

## Initialization
```c
initscr();  // Initializes "cursor mode"
cbreak();   // Reads control signals
noecho();   // Prevents echoing input to terminal

int max_x, max_y;
getmaxyx(stdscr, max_y, max_x); // Gets the terminal rows and columns sizes

int chat_height = max_y - 2; // Reserve some space for input

// ...
endwin();   // Exits "cursor mode"

```

The documentation on `cbreak`:
> The difference between these two functions is in the way control characters like suspend (CTRL-Z), interrupt and quit (CTRL-C) are passed to the program. In the raw() mode these characters are directly passed to the program without generating a signal. In the cbreak() mode these control characters are interpreted as any other character by the terminal driver. I personally prefer to use raw() as I can exercise greater control over what the user does.

Basically, when you call `cbreak`, you are telling ncurses to NOT intercept control signals.

The documentation on `noecho`:
> noecho() switches off echoing. The reason you might want to do this is to gain more control over echoing or to suppress unnecessary echoing while taking input from the user through the getch()

Basically, you want to be able to control how characters get echoed to the terminal, so you should call this during initialization.

## Input

Method for clearing a window:
```c
for (int i = 0; i < chat_height; i++) {
    move(i, 0);
    clrtoeol();
}
```

Method for printing chat contents to window:
```c
for (int i = msg_start; i < msg_end; i++) {
    if (line > chat_height) break;
    move(line++, 0);
    printw("[%s] %s: %s", messages[i].timestamp, messages[i].username, messages[i].text);
}
```


Method for printing user input prompt to bottom of window:
```c
move(max_y - 2, 0);
clrtoeol();
for (int i = 0; i < max_x; i++) addch('-');
```

Note: It might be more sensible to just but the prompt and the message history in two seperate windows to avoid all this hacky math and rerendering.

- Need to use `refresh()` to actually dump writes to the terminal. Should perform this everytime a rerender should occur.
- Use `getch()`, which blocks until the user types a character, to read terminal input from the user.

## Multiple windows

A great explaination of handling multiple windows: [Docs](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/awordwindows.html)

The highlights:
- The default window, `stdscr`, is controlled with the default functions (`printw`, `mvprintw`, `refresh`, etc)
- Other windows are controlled with variants of these basic functions (`wprintw`, `wmvprintw`, `wrefresh`, etc) and work much like the `stdio` family of functions.

> Usually the w-less functions are macros which expand to corresponding w-function with stdscr as the window parameter.

## Forms

TODO: Write about forms