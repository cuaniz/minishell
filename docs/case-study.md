# minishell case study

## Context

`minishell` started as a university systems-programming project at ESCOM IPN. The goal was to build a small shell in C and understand how command execution works under the hood instead of treating the terminal as a black box.

## Problem

A shell looks simple from the outside, but it coordinates several operating-system concepts at once: processes, file descriptors, pipes, terminal state, signals, and user input. This project focuses on making those concepts concrete in a working program.

## Objective

Build a minimal interactive shell that can execute commands, manage a current directory, support common redirection and pipe operators, and remain understandable for code review.

## My role

I designed and implemented the shell as the developer of the project. The work includes parsing user input, executing child processes, wiring pipes and redirections, handling basic terminal input behavior, and preparing the repository for professional review.

## Main technical decisions

- **Use C and POSIX APIs directly.** This makes process management and file-descriptor flow explicit.
- **Keep the project small.** The repository is easier to review when it focuses on the core shell behavior instead of growing into an incomplete Bash clone.
- **Document limitations honestly.** Recruiters and developers can evaluate the project faster when the scope is clear.
- **Add reproducible commands.** `make`, `make check`, and `.env.example` make the repository easier to clone and validate.

## Challenges

- Managing file descriptors correctly when combining pipes and redirections.
- Preserving an interactive prompt while still letting child processes receive default signal behavior.
- Handling terminal input for history navigation without relying on a large shell framework.
- Keeping parsing simple while still supporting useful operators.

## Validation

The project was compiled and executed successfully on Ubuntu in my personal development environment, which matches the intended UNIX/POSIX scope for an Operating Systems course project.

## What I learned

- How `fork`, `execvp`, `pipe`, `dup2`, and `wait` cooperate to execute shell commands.
- Why terminal state must be restored after changing canonical/echo modes.
- Why repository hygiene matters: reviewers need clear docs, reproducible commands, and no generated or sensitive files.
- Why professional code is not about pretending everything is perfect; it is about making tradeoffs visible and maintainable.

## What I would improve next

- Add a small automated test harness that runs in Linux CI.
- Improve parsing for escaped characters, environment-variable expansion, and more robust quote handling.
- Add job control for background processes.
- Split the implementation into smaller modules once the project grows beyond a single educational file.

## Why this demonstrates useful junior-developer skills

This project shows that I can work close to the operating system, reason about process control, document tradeoffs, and prepare a repository so another developer can clone it, build it, and understand it quickly. Those habits matter in a first professional role because maintainability and communication are as important as writing code that works locally.
