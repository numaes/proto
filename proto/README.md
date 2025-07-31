# Proto: A High-Performance, Embeddable Dynamic Object System for C++

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://isocpp.org/)
[![Build System](https://img.shields.io/badge/Build-Makefile-green.svg)](https://www.gnu.org/software/make/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE.md)

**Proto is a powerful, embeddable runtime written in modern C++ that brings the flexibility of dynamic, prototype-based object systems (like those in JavaScript or Python) into the world of high-performance, compiled applications.**

It is designed for developers who need to script complex application behavior, configure systems dynamically, or build domain-specific languages without sacrificing the speed and control of C++. With Proto, you get an elegant API, automatic memory management, and a robust, immutable data model.

---

## Core Features

*   **Dynamic Typing in C++**: Create and manipulate integers, floats, booleans, strings, and complex objects without compile-time type constraints.
*   **Prototypal Inheritance**: A flexible and powerful object model. Objects inherit directly from other objects, allowing for dynamic structure, mixins, and behavior sharing without the rigidity of classical inheritance.
*   **Immutable Data Structures**: Collections like lists, tuples, and dictionaries are immutable by default. Operations like `append` or `set` return new, modified versions, eliminating a whole class of bugs related to shared state and making concurrent programming safer and easier to reason about.
*   **Automatic Memory Management**: A concurrent, stop-the-world garbage collector manages the lifecycle of all objects, freeing you from manual `new` and `delete`.
*   **Thread-Safe by Design**: Core operations are designed with concurrency in mind, featuring a managed thread model and safe memory allocation.
*   **Clean, Embeddable C++ API**: The entire system is exposed through a clear and minimal public API (`proto.h`), making it easy to integrate into existing C++ applications.

## Getting Started

Getting up and running with Proto is simple. All you need is a C++20 compatible compiler (like `g++`) and `make`.

### 1. Prerequisites

Ensure you have `g++` and `make` installed on your system.
