# minishell

A small POSIX-style command-line shell written in C. It was built as a university systems-programming project and polished for portfolio review: the code demonstrates process creation, pipes, redirections, terminal input handling, and basic shell built-ins without hiding the current learning-stage limitations.

## What it does

`minishell` gives users a simple interactive shell where they can run external commands, navigate directories, use command history, and combine commands through common shell operators.

## Features

- Interactive prompt with current user and working directory.
- External command execution with `fork` and `execvp`.
- Built-in `cd` and `exit` commands.
- Command history navigation with arrow keys.
- Input and output redirection: `<`, `>`, `>>`.
- Pipes between commands with `|`.
- Basic heredoc support with `<<`.
- `Ctrl+C` signal handling for the shell prompt.

## Tech stack

| Area | Technology |
| --- | --- |
| Language | C |
| Platform APIs | POSIX (`fork`, `execvp`, `pipe`, `dup2`, `termios`, signals) |
| Build | `make` + `gcc` or compatible C compiler |
| Tests/checks | Compiler warnings and a smoke-test script |

## Requirements

This project targets POSIX environments such as Linux, macOS, or WSL with a real Linux distribution.

Native Windows MinGW is not enough because the program depends on POSIX headers such as `sys/wait.h` and terminal APIs.

The program has been compiled and executed successfully on Ubuntu in a personal development environment.

## Installation

```sh
git clone <repository-url>
cd minishell
make
```

## Run

```sh
./minishell
```

Example session:

```sh
pwd
echo hello
ls -la | grep minishell
echo output > example.txt
cat << EOF
multi-line input
EOF
cd ..
exit
```

## Useful commands

```sh
make        # build the binary
make check  # build and run the smoke test
make clean  # remove generated build artifacts
```

## Environment variables

No project-specific environment variables are required.

The shell reads standard environment variables already provided by the operating system:

| Variable | Purpose |
| --- | --- |
| `USER` | Used to display the prompt user name when available. |
| `HOME` | Used by `cd` when no directory argument is provided. |

## Demo data

No database or seed data is needed. The project is self-contained.

## License

This project is licensed under the MIT License. See `LICENSE` for details.

## Current status

Portfolio-ready educational project. The repository is intentionally small and focused on operating-system fundamentals rather than a full production shell.

Validation status: compiled and manually executed on Ubuntu.

## Technical decisions

- Kept the implementation in C to practice low-level process and file-descriptor management directly.
- Used POSIX system calls instead of wrapping everything in external libraries, so the core behavior is visible to reviewers.
- Added a Makefile and smoke test to make the project easier to clone, build, and review.
- Documented platform limitations clearly instead of pretending the program is portable to native Windows.

## Known limitations

- It is not a full Bash-compatible shell.
- Parsing is intentionally simple: advanced quoting, escaping, variable expansion, globbing, and job control are out of scope.
- The shell is best evaluated in a Linux/macOS/WSL terminal, not in native Windows terminals using MinGW.

These limitations are documented because they define the project scope; they are not hidden defects.

## Portfolio note

This project shows practical understanding of C, POSIX processes, file descriptors, terminal handling, error handling, and repository hygiene. It is a good fit for demonstrating systems-programming fundamentals for an entry-level developer role.
