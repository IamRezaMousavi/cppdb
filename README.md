# CppDB

A modern, lightweight C++11 SQL database library with a unified API for multiple relational database management systems.

CppDB provides high-performance database access through connection pooling, prepared statement caching, and seamless C++ type mapping while maintaining a simple and intuitive API.

---

## Features

### Modern C++

* Modern C++11 implementation
* RAII-based resource management
* Smart pointer-based memory management
* Exception-based error handling
* Cross-platform support (Linux, Windows, macOS)

### Performance

* High-performance connection pooling
* Prepared statement caching
* Low-overhead database access
* Efficient resource utilization

### SQL API

* Prepared statements
* Parameter binding
* Transaction management
* Result set iteration
* Automatic statement execution helpers

### Type Mapping

* Automatic mapping between SQL and C++ types
* Built-in support for primitive types
* `std::string` support
* Date and time support (`std::tm`)
* NULL value handling
* Extensible type conversion system for user-defined types

### Reliability

* Thread-safe connection pool
* Locale-safe string handling
* Automatic resource cleanup
* Consistent exception reporting

### Build System

* Modern CMake build system
* Easy integration with existing CMake projects
* Optional system-wide installation
* Minimal external dependencies

---

## Supported Databases

| Database | Required Client Library |
| -------- | ----------------------- |
| SQLite3 | SQLite3 |
| MySQL / MariaDB | libmysqlclient |
| PostgreSQL | libpq |
| ODBC | unixODBC (Linux/macOS) or Windows ODBC API |

Additional database systems can be accessed through the ODBC backend.

---

## Requirements

* C++11 compatible compiler
* CMake 3.10+
* Database client libraries for the selected backend

---

## Installation

### Using CMake

```cmake
add_subdirectory(external/cppdb)

target_link_libraries(my_application PRIVATE cppdb)
```

### Build

```bash
cmake -B build
cmake --build build
```

### Install

```bash
cmake --install build
```

---

## Quick Example

Here’s a very simple example:

```cpp
#include <cppdb/frontend.hpp>
#include <iostream>

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
            << 10
            << 3.1415926565
            << now
            << "Hello 'World'";

        stat.exec();
        std::cout << "ID: " << stat.last_insert_id() << std::endl;
        std::cout << "Affected rows " << stat.affected() << std::endl;
        std::cout << "Data added successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

---

## Connection String Examples

### SQLite

```text
sqlite3:db=test.db
```

### MySQL

```text
mysql:database=test;user=root;password=secret
```

### PostgreSQL

```text
postgresql:dbname=test;user=postgres;password=secret
```

### ODBC

```text
odbc:dsn=my_database
```

---

## Roadmap

* Broader C++ Standard Library (STL) Suppor
* Improved documentation
* Extended type mapping
* Additional database backends
* Performance optimizations

---

## Contributing

Contributions, bug reports, feature requests, and pull requests are welcome.

Please open an issue before making large or breaking changes.

---

## License

This project is licensed under the **[GNU Lesser General Public License v2.1 (LGPL-2.1)](./LICENSE)**.

---

## Acknowledgements

* Thanks to the original [cppdb](http://cppcms.com) developers for creating the foundational library.
* Thanks to the C++ community for the modern tools and standards provided.

---
