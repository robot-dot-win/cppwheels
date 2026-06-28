[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/robot-dot-win/cppwheels)

cppwheels
=========

1\. Project Overview
--------------------

`cppwheels` is a static utility library engineered with the C++20 standard. It provides an optimized, generalized, and highly reusable set of infrastructural components designed to supplement the C++ Standard Library. The repository focuses on addressing recurring computational patterns in micro and small-scale software architecture, strictly avoiding heavyweight third-party dependencies (e.g., Boost, ICU) to minimize compilation overhead and structural complexity.

2\. Module Architecture & Capabilities
--------------------------------------

The repository is systematically divided into several functional domains based on the underlying data types and operations:

### 2.1 String Processing & Manipulation (`strext`, `strjoin`)

* **Lexical Tokenization**: Implements rigorous string splitting (`splits`, `splitsv`) with highly optimized delimiter caching (`splitiv`). Supports bitwise behavioral options (trimming, discarding empty tokens).

* **Mutation & Trimming**: Provides in-place and returning string manipulation functions (case conversion, `ltrim`/`rtrim`/`trim`). Extensively utilizes `std::string_view` to ensure zero-copy semantics where applicable.

* **Sub-string Replacement**: Utilizes the Boyer-Moore search algorithm (`std::boyer_moore_searcher`) for high-performance, non-overlapping string replacement (`replall`).

* **Zero-Copy Concatenation**: Features a lazy evaluation mechanism (`strjoin`) for joining containers of strings. It automatically dispatches to C++23 `std::views::join_with` if supported, falling back to a custom C++20 standard forward iteration proxy.

* **Security Utilities**: Includes configurable password generation (`genPassword`) and validation (`chkPassword`) algorithms based on entropic requirements.

### 2.2 Generic Container Algorithms (`algext`)

* **Cost-Aware Container Operations**: Implements generic algebraic operations (Difference, Union) for heterogeneous C++ containers (`SequentialContainer`, `AssociativeContainer`).

* **Heuristic Subtraction (`subContainer`)**: Dynamically computes algorithmic complexity parameters at compile-time/run-time (evaluating Scan-A, Hash-B, and Scan-B strategies) to select the optimal time-complexity path for element removal.

* **Node-Based Transfer (`mvtoContainer`)**: Utilizes C++17 node extraction (`extract()`) to perform zero-copy memory transfers between associative containers.

### 2.3 Time & Chronometry (`chronoext`)

* **Timepoint Formatting**: Wraps `std::chrono` functionalities, bridging the C++20 `std::chrono::zoned_time` capabilities for precise date and time-string generation.

* **Timeout Management**: Provides a stateful `TTimeout` class to execute deterministic expiration evaluations (`expires()`), coupled with a `TimeoutManager` for centralized chronometric state tracking.

### 2.4 Network Protocol Utilities (`ip4`)

* **IPv4 Semantics**: Encapsulates robust IPv4 address processing, ensuring strict compliance with system endianness (`__BYTE_ORDER__`). Supports bitwise shifting, incrementation, and comparison operations.

* **CIDR Routing Logic**: The `ip4net` module parses CIDR notations (e.g., `/24`) to compute network masks, subnet identifiers, and broadcast addresses algebraically.

### 2.5 System & Sub-process Execution (`procext`)

* **Cross-Platform Sub-process Management**: Abstracts POSIX `popen`/`pclose` and Windows `_popen`/`_pclose` via a unified execution interface.

* **Stream Processing**: Features `cmdoutputline`, a C++20 `std::invocable`\-constrained template that processes standard output streams line-by-line via functional callbacks, capturing exit codes and diagnostic signals synchronously.

### 2.6 Data Formats & Identifiers (`cfgfile`, `nejson`, `ltuuid`)

* **Configuration Parsing (`cfgfile`)**: Evaluates INI/CFG-style syntax, supporting section-based key-value mappings and line-comment ignoring rules.

* **Deterministic JSON (`nejson`)**: A "No-Exception" architectural wrapper around `nlohmann::json`, guaranteeing exception-free JSON traversal (`value`, `get_to`) by enforcing fallback default values.

* **UUID Generation (`ltuuid`)**: Implements an RFC 4122 compliant UUID v4 generator utilizing a thread-local `std::mt19937_64` engine. Provides integration with C++20 `std::format` via specialized formatters.

3\. System Requirements & Build Instructions
--------------------------------------------

* **Compiler Specification**: Full compliance with the C++20 standard is strictly required (`cxx_std_20`).

* **Build System**: Configured via CMake (`CMakeLists.txt`) to output a static library artifact (`cppwheels`).

