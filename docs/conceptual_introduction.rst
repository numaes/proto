Conceptual Introduction to Proto Abstractions
=============================================

Proto is designed to be a flexible and high-performance runtime for new programming languages. At its core, it provides a set of fundamental abstractions that language designers can leverage to build their own unique language semantics. Understanding these core components is key to effectively utilizing Proto.

The `proto.h` header file exposes the primary interfaces you'll interact with when building a language on Proto. Let's explore some of the most important ones:

ProtoObject: The Universal Base
-------------------------------

At the heart of Proto is the :cpp:class:`proto::ProtoObject` class. Every single entity in a Proto-based language, from numbers and strings to functions and custom data structures, is ultimately a `ProtoObject`. This provides a unified and consistent way to interact with all data within the runtime.

Think of `ProtoObject` as the `Object` class in languages like Java or Python. It provides fundamental behaviors such as:

*   **Cloning and Instantiation**: :cpp:func:`proto::ProtoObject::clone` and :cpp:func:`proto::ProtoObject::newChild` allow you to create copies or new instances based on existing objects.
*   **Attribute Management**: Methods like :cpp:func:`proto::ProtoObject::getAttribute`, :cpp:func:`proto::ProtoObject::setAttribute`, and :cpp:func:`proto::ProtoObject::hasAttribute` enable dynamic property access, crucial for object-oriented or prototype-based languages.
*   **Method Invocation**: The :cpp:func:`proto::ProtoObject::call` method is the entry point for executing behavior associated with an object, supporting flexible dispatch mechanisms.
*   **Type Coercion**: Methods like :cpp:func:`proto::ProtoObject::asInteger`, :cpp:func:`proto::ProtoObject::asBoolean`, and :cpp:func:`proto::ProtoObject::asDouble` allow objects to be interpreted as fundamental data types when appropriate.

**Why this matters to you:**
As a language designer, you'll define your language's types by extending `ProtoObject`. This powerful abstraction means you don't have to reinvent basic object behaviors; you can focus on the unique aspects of your language.

Cell: The Atomic Memory Unit
----------------------------

While `ProtoObject` defines behavior, :cpp:class:`proto::Cell` represents the fundamental unit of memory allocation within Proto. All `ProtoObject` instances, and indeed most internal data structures, are composed of or reside within `Cell` objects.

Key characteristics of `Cell`:

*   **Fixed Size**: Cells are designed to be small and fixed-size (e.g., 64 bytes). This design choice is critical for performance and efficient memory management, especially in highly concurrent environments.
*   **Non-Mutable (mostly)**: Once initialized, `Cell` objects are largely immutable, except for specific pointers like `context` and `nextCell` which can be atomically updated. This immutability simplifies concurrency and garbage collection.
*   **Page-Aligned Allocation**: Cells are allocated in OS page-sized chunks, optimizing memory access and cache utilization.

**Why this matters to you:**
You typically won't directly manipulate `Cell` objects. Instead, Proto's runtime handles their allocation and management. However, understanding that your language's objects are built from these atomic, immutable units helps in designing efficient data structures and understanding Proto's performance characteristics.

ProtoContext: The Execution Environment
---------------------------------------

The :cpp:class:`proto::ProtoContext` is your window into the current execution state. It's passed around almost every operation and provides access to:

*   **Local Variables**: Manages the stack frame and local variables for method calls.
*   **Object Creation**: Provides factory methods (e.g., :cpp:func:`proto::ProtoContext::fromInteger`, :cpp:func:`proto::ProtoContext::fromUTF8String`, :cpp:func:`proto::ProtoContext::newList`) to create new `ProtoObject` instances of various types.
*   **Thread Management**: Links to the current :cpp:class:`proto::ProtoThread` of execution.
*   **Space Management**: Provides access to the global :cpp:class:`proto::ProtoSpace`.

**Why this matters to you:**
When implementing your language's built-in functions or methods, the `ProtoContext` is your primary tool for interacting with the runtime, creating objects, and managing execution flow.

ProtoSpace: The Universe of Your Language
-----------------------------------------

The :cpp:class:`proto::ProtoSpace` represents the entire runtime environment for your language. It's the "universe" where all objects live and all execution happens.

`ProtoSpace` is responsible for:

*   **Prototypes**: Holds references to the prototype objects for all built-in types (e.g., `objectPrototype`, `listPrototype`, `stringPrototype`). These are the foundational objects from which all other instances of that type are derived.
*   **Garbage Collection**: Manages memory reclamation, ensuring efficient use of resources.
*   **Thread Management**: Oversees all active :cpp:class:`proto::ProtoThread` instances.
*   **Global State**: Manages global literals and other shared resources.

**Why this matters to you:**
When you initialize your language, you'll create a `ProtoSpace`. This is where you define the initial state of your language, including its fundamental types and global objects.

Core Data Structures: Lists, Tuples, Strings, and Sparse Lists
--------------------------------------------------------------

Proto provides highly optimized implementations of common data structures, exposed through interfaces like:

*   :cpp:class:`proto::ProtoList`: A mutable, ordered collection of ProtoObjects, similar to Python lists or JavaScript arrays.
*   :cpp:class:`proto::ProtoTuple`: An immutable, ordered collection, akin to Python tuples.
*   :cpp:class:`proto::ProtoString`: An immutable sequence of characters, optimized for UTF-8.
*   :cpp:class:`proto::ProtoSparseList`: A key-value store where keys are unsigned long integers, useful for sparse arrays or dictionaries.

Each of these comes with its own iterator classes (e.g., :cpp:class:`proto::ProtoListIterator`) for efficient traversal.

**Why this matters to you:**
These built-in data structures provide a high-performance foundation for your language's collections. You can directly use them or build more complex data structures on top of them.

ProtoMethod and ParentLink: Functionality and Inheritance
---------------------------------------------------------

*   :cpp:type:`proto::ProtoMethod`: This is a function pointer type that defines the signature for methods in your Proto-based language. It allows for dynamic dispatch and execution of code.
*   :cpp:class:`proto::ParentLink`: This abstraction is crucial for implementing inheritance and prototype-based object models. It represents a chain of parent objects, allowing Proto to resolve attribute lookups and method calls through an object's inheritance hierarchy.

**Why this matters to you:**
These components are fundamental for defining the behavior of your objects and how they relate to each other through inheritance or delegation.

Experiment and Explore!
-----------------------

The best way to understand Proto's abstractions is to experiment. Consider these ideas:

*   **Implement a simple calculator language**: Define `Number` and `Operator` objects, and use `ProtoMethod` to implement arithmetic operations.
*   **Build a basic object system**: Use `ProtoObject` and its attribute management methods to create a simple prototype-based object model.
*   **Explore data structures**: Create instances of `ProtoList` and `ProtoString` and experiment with their methods to understand their performance characteristics.
*   **Custom memory management**: While Proto handles most memory, you could experiment with `ProtoByteBuffer` for direct byte manipulation for specific use cases.

By diving into the code and building small examples, you'll quickly grasp the power and flexibility that Proto offers for language development.
