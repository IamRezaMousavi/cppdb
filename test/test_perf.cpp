///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2010-2011  Artyom Beilis (Tonkikh) <artyomtnk@yahoo.com>
//
//  Distributed under:
//
//                   the Boost Software License, Version 1.0.
//              (See accompanying file LICENSE_1_0.txt or copy at
//                     http://www.boost.org/LICENSE_1_0.txt)
//
//  or (at your opinion) under:
//
//                               The MIT License
//                 (See accompanying file MIT.txt or a copy at
//              http://www.opensource.org/licenses/mit-license.php)
//
///////////////////////////////////////////////////////////////////////////////
#include <cppdb/frontend.h>

#include <stdlib.h>

#include <iostream>

#if defined WIN32 || defined _WIN32 || defined __WIN32 || defined(__CYGWIN__)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class timer {
public:
	timer() {
		QueryPerformanceFrequency(&freq_);
	}
	void start() {
		QueryPerformanceCounter(&start_);
	}
	void stop() {
		QueryPerformanceCounter(&stop_);
	}
	double diff() const {
		return double(stop_.QuadPart - start_.QuadPart) / freq_.QuadPart;
	}

private:
	LARGE_INTEGER freq_;
	LARGE_INTEGER start_;
	LARGE_INTEGER stop_;
};

#else

#include <sys/time.h>

class timer {
public:
	timer() {}
	void start() {
		gettimeofday(&start_, 0);
	}
	void stop() {
		gettimeofday(&stop_, 0);
	}
	double diff() const {
		double udiff = (stop_.tv_sec - start_.tv_sec) * 1000000LL + stop_.tv_usec - start_.tv_usec;
		return udiff * 1e-6;
	}

private:
	struct timeval start_;
	struct timeval stop_;
};

#endif

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "[ERROR] Connection string required\n";
		return 1;
	}
	try {
		static const int max_val = 10000;
		const std::string cs = argv[1];
		cppdb::session sql(cs);

		std::cout << "===== CppDB Performance Test Report =====\n";
		std::cout << "Connection: " << cs << '\n';
		std::cout << "Engine: " << sql.engine() << '\n';
		std::cout << "Records: " << max_val << '\n';
		std::cout << "Iterations: " << max_val * 10 << '\n';
		std::cout << "============================\n";

		try {
			sql << "DROP TABLE test" << cppdb::exec;
		} catch (...) {}

		if (sql.engine() == "mysql")
			sql << "create table test ( id integer primary key, val varchar(100)) Engine=innodb" << cppdb::exec;
		else
			sql << "create table test ( id integer primary key, val varchar(100))" << cppdb::exec;

		std::cout << "[SETUP] Table created\n";

		{
			cppdb::transaction tr(sql);
			for (int i = 0; i < max_val; i++) {
				sql << "insert into test values(?,?)" << i << "Hello World" << cppdb::exec;
			}
			tr.commit();
		}

		std::cout << "[INSERT] Inserted " << max_val << " records\n";

		timer tm;

		tm.start();

		for (int j = 0; j < max_val * 10; j++) {
			std::string v;
			sql << "select val from test where id = ?" << (rand() % max_val) << cppdb::row >> v;
			if (v != "Hello World")
				throw std::runtime_error("Wrong");
		}
		tm.stop();

		std::cout << "[SELECT] Completed " << max_val * 10 << " queries\n";
		std::cout << "[TIME] " << tm.diff() << " seconds\n";
	} catch (const std::exception &e) {
		std::cerr << "[ERROR] " << e.what() << '\n';
		return 1;
	}
	std::cout << "===== RESULT: OK =====\n";
	return 0;
}
