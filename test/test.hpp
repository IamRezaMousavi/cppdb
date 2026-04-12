#ifndef CPPDB_TEST_H
#define CPPDB_TEST_H
#include <stdlib.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

int last_line = 0;
int passed = 0;
int failed = 0;

#define TEST(x)                                                          \
	do {                                                                 \
		last_line = __LINE__;                                            \
		if (x) {                                                         \
			passed++;                                                    \
			break;                                                       \
		}                                                                \
		failed++;                                                        \
		std::cerr << "Failed in " << __LINE__ << ' ' << #x << std::endl; \
	} while (0)

#define THROWS(x, ex)                                                        \
	do {                                                                     \
		last_line = __LINE__;                                                \
		try {                                                                \
			x;                                                               \
			failed++;                                                        \
			std::cerr << "Failed in " << __LINE__ << ' ' << #x << std::endl; \
		} catch (ex const & /*un*/) {                                        \
			passed++;                                                        \
		}                                                                    \
	} while (0)
#endif

#define CATCH_BLOCK()                                      \
	catch (std::exception const &e) {                      \
		std::cerr << "[FATAL ERROR] " << e.what() << '\n'; \
	}
#define SUMMARY()                                                                               \
	do {                                                                                        \
		std::cout << "[SUMMARY] Tests: " << passed + failed << " | Failed: " << failed << '\n'; \
		if (failed > 0) {                                                                       \
			std::cout << "[RESULT] FAILED\n";                                                   \
			return 1;                                                                           \
		}                                                                                       \
		std::cout << "[RESULT] ALL TESTS PASSED\n";                                             \
		return 0;                                                                               \
	} while (0)
