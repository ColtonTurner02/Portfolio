# Bash Shell Mimic in C

A simple shell implementation in **C** that mimics the core functionality of the Unix **bash** shell.  

This project was built from scratch to explore process management, inter-process communication, and signal handling in Unix-like environments.  

## Features
- Implements several built-in commands similar to the Bash shell  
- Supports **serial**, **parallel**, and **background** execution of forked processes  
- Demonstrates process creation, child management, and signal handling  

## Getting Started

### Compilation
```bash
gcc myshell.c -o myshell
