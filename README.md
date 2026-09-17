# SyncForge

A multithreaded file transfer and synchronization system
built using C and Linux.

## Features

- TCP file transfer
- Multiple clients
- Thread pool
- Producer-consumer queue
- File upload/download
- Hash table based file index
- Linux filesystem APIs

## Requirements

- Linux
- GCC
- pthread

## Build

make

## Run Server

./syncforge server 8080

## Run Client

./syncforge client 127.0.0.1 8080