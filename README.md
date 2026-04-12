# CppDB: Modern & Lightweight C++ Database Library

This repository is a modern, updated fork of the `cppdb` library, created with the goal of providing a better development experience, leveraging new C++ features, and improving performance and memory management. By focusing on modern standards, this version offers a lightweight and efficient solution for working with databases in C++ projects.

---

## About This Fork

The original [cppdb](https://sourceforge.net/projects/cppcms/files/) library, while powerful, has been unmaintained for years and has become outdated with respect to modern C++ features and best development practices. This fork addresses these issues by providing a modernized and enhanced version of cppdb.

Our primary goals with this fork are:

* **Modernize the codebase:** We fully utilize C++20 features and have removed outdated code and deprecated methods, replacing them with standard, modern C++ equivalents.
* **Enhance Performance and Memory Management:** We employ modern techniques, including the use of Smart Pointers, for more efficient memory handling, a reduced risk of memory leaks, and overall improved performance.
* **Simplify and Improve Readability:** The API has been refined to be cleaner, more straightforward, and easier to understand and use, making development faster and less error-prone.
* **Streamline Portability and Dependencies:** We’ve simplified the build process using CMake and minimized dependencies to standard C++ libraries and essential database connectors, ensuring easier compilation across different platforms.
* **Broaden Type Support:** The library now seamlessly supports newer and more advanced data types, offering greater flexibility.

Based on these goals, this fork offers the following Key Features:

* **Performance-Oriented:** Performance is the primary objective, aiming for the fastest possible SQL connectivity.
* **Connection Pooling:** Transparent support for connection pooling is included to optimize database interactions.
* **Prepared Statements Caching:** Efficient caching of prepared statements reduces repetitive parsing and execution overhead.
* **Dynamic Module Loading:** Supports dynamic loading of database modules and optional static linking for flexibility.
* **Comprehensive RDBMS Support:** Full and high-priority support for Free and Open Source Software (FOSS) RDBMS: MySQL, PostgreSQL, and SQLite3. Extensive support for a wider range of RDBMSs is available through the cppdb-odbc bridge.
* **Ease of Use:** Designed for simplicity, making it straightforward to integrate and use.
* **Locale Safety:** Ensures correct handling of international characters and data.
* **Flexible API:** Offers both an explicit, verbose API for detailed control and a brief, convenient syntactic sugar for quicker development.

---

## Installation & Setup

### Prerequisites

* A C++ compiler with C++20 support (e.g., GCC 10+, Clang 10+, MSVC 19.29+).
* CMake version 3.16 or higher.
* Relevant drivers for your target database.
  * Sqlite3 - the library and headers themselfs
  * MySQL - libmysqlclient
  * PostgreSQL - libpq
  * ODBC - unixodbc for POSIX operating systems and Window's build in odbc API for windows.

### Via CMake

The easiest way to add `cppdb` to your project is by using CMake:

1. Clone this repo

2. Add it as your project dependency

    ```cmake
    add_subdirectory(path/to/cppdb/directory) # Path to cppdb source directory within your project
    target_link_libraries(your_project_target PRIVATE cppdb)
    ```

### System Installation

Optional: You can add instructions for system-wide installation using make install if desired, although CMake is usually sufficient.

1. Clone this repo

2. Compile it (e.g., in a build directory):

    ```bash
    cmake -Bbuild && cmake --build build
    ```

3. Install it.

    ```bash
    cmake --install build
    ```

---

## Usage

For complete and practical examples of how to use the modernized cppdb, please refer to the [examples/](./examples/) directory in this repository. You will find examples demonstrating connection to various databases, executing queries, and utilizing the new features.

Here’s a very simple example:

```cpp
#include <cppdb/frontend.hpp>
#include <iostream>
#include <string>

int main() {
    try {
        cppdb::session sql("sqlite3:db=database.db");

        sql << "DROP TABLE IF EXISTS test" << cppdb::exec;

        sql << "CREATE TABLE test ( "
            "   id   INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, "
            "   n    INTEGER,"
            "   f    REAL, "
            "   t    TIMESTAMP,"
            "   name TEXT "
            ")  "
        << cppdb::exec;

        std::time_t now_time = std::time(0);
        std::tm now = *std::localtime(&now_time);

        cppdb::statement stat;
        stat = sql << "INSERT INTO test(n,f,t,name) "
            "VALUES(?,?,?,?)"
            << 10 << 3.1415926565 << now << "Hello 'World'";

        stat.exec();
        std::cout << "ID: " << stat.last_insert_id() << std::endl;
        std::cout << "Affected rows " << stat.affected() << std::endl;

        stat.reset();

        stat.bind(20);
        stat.bind_null();
        stat.bind(now);
        stat.bind("Hello 'World'");
        stat.exec();

        std::cout << "Data added successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

---

## Contributing

We welcome your contributions to improve the code! If you’d like to help, please:

1. Fork the repository.
2. Create a new branch for your feature or bug fix (git checkout -b feature/AmazingFeature).
3. Commit your changes (git commit -m 'Add some AmazingFeature').
4. Push to the branch (git push origin feature/AmazingFeature).
5. Submit a Pull Request.

Please check the issues for any existing bug reports before submitting a Pull Request.

---

## License

This project is licensed under the **GNU Lesser General Public License v2.1 (LGPL-2.1)**.

Please see the [LICENSE](./LICENSE) file for more details.

---

## Acknowledgements

* Thanks to the original [cppdb](http://cppcms.com) developers for creating the foundational library.
* Thanks to the C++ community for the modern tools and standards provided.

---
