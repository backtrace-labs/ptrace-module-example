This directory contains an example ptrace module, its Makefile, and an example
C++ program that demonstrates usage.

To run the example, you need to do the following:
- Build the C++ program
- Build the ptrace module
- Run the program
- Run ptrace on the program

# Build the C++ program

For example:
```
g++ -std=c++11 program.cpp -ggdb -o program
```

# Build the ptrace module

```
make
```

# Run program

```
./program &
```

# Run ptrace on the program

Assuming PID of the program is 1337

```
ptrace 1337 --pretty-print --module-load=pmodule-example.so
```
This should leave you with a `.btt` file in the current working directory. It
can be later inspected by `hydra`, `ptrace`, or uploaded to your coroner
instance using the Web-UI or `morgue`.