CExec

CExec is a Linux-based C code execution server built from scratch using C and POSIX APIs.

Clients send C source files over TCP. The server stores, compiles, executes, and returns the program output while handling multiple clients concurrently.

Features
TCP client-server communication
Custom binary protocol
Multithreaded server using pthread
File upload and storage
Per-client workspaces
fork() + exec() based execution
waitpid() process management
CPU and memory limits using setrlimit()
Execution timeout using kill()
SHA-256 file integrity
File operations: UPLOAD, LIST, DOWNLOAD
