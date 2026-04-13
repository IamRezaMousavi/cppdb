#define CPPDB_DRIVER_SOURCE
#ifdef CPPDB_WITH_SQLITE3
#define CPPDB_SOURCE
#endif

#include <cppdb/backend.hpp>
#include <cppdb/errors.hpp>
#include <cppdb/utils.hpp>

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>

#include <iomanip>
#include <limits>
#include <map>
#include <sstream>

namespace cppdb {
namespace sqlite3_backend {

class result : public backend::result {
public:
	result(sqlite3_stmt *st, sqlite3 *conn) : st_(st), conn_(conn), column_names_prepared_(false), cols_(-1) {
		cols_ = sqlite3_column_count(st_);
	}
	virtual ~result() {
		st_ = 0;
	}
	virtual next_row has_next() {
		return next_row_unknown;
	}
	virtual bool next() {
		int r = sqlite3_step(st_);
		if (r == SQLITE_DONE)
			return false;
		if (r != SQLITE_ROW) {
			throw cppdb_error(std::string("sqlite3:") + sqlite3_errmsg(conn_));
		}
		return true;
	}
	template <typename T>
	bool do_fetch(int col, T &v) {
		if (do_is_null(col))
			return false;
		if (sqlite3_column_type(st_, col) == SQLITE_NULL)
			return false;
		sqlite3_int64 rv = sqlite3_column_int64(st_, col);
		T tmp;
		if (std::numeric_limits<T>::is_signed) {
			tmp = static_cast<T>(rv);
			if (static_cast<sqlite3_int64>(tmp) != rv)
				throw bad_value_cast();
		} else {
			if (rv < 0)
				throw bad_value_cast();
			unsigned long long urv = static_cast<unsigned long long>(rv);
			tmp = static_cast<T>(urv);
			if (static_cast<unsigned long long>(tmp) != urv)
				throw bad_value_cast();
		}
		v = tmp;
		return true;
	}

	virtual bool fetch(int col, short &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, unsigned short &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, int &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, unsigned &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, long &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, unsigned long &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, long long &v) {
		return do_fetch(col, v);
	}
	virtual bool fetch(int col, unsigned long long &v) {
		return do_fetch(col, v);
	}
	template <typename T>
	bool do_real_fetch(int col, T &v) {
		if (do_is_null(col))
			return false;
		v = static_cast<T>(sqlite3_column_double(st_, col));
		return true;
	}
	virtual bool fetch(int col, float &v) {
		return do_real_fetch(col, v);
	}
	virtual bool fetch(int col, double &v) {
		return do_real_fetch(col, v);
	}
	virtual bool fetch(int col, long double &v) {
		return do_real_fetch(col, v);
	}
	virtual bool fetch(int col, std::string &v) {
		if (do_is_null(col))
			return false;
		const char *txt = (const char *)sqlite3_column_text(st_, col);
		int size = sqlite3_column_bytes(st_, col);
		v.assign(txt, size);
		return true;
	}
	virtual bool fetch(int col, std::ostream &v) {
		if (do_is_null(col))
			return false;
		const char *txt = (const char *)sqlite3_column_text(st_, col);
		int size = sqlite3_column_bytes(st_, col);
		v.write(txt, size);
		return true;
	}
	virtual bool fetch(int col, std::tm &v) {
		if (do_is_null(col))
			return false;
		v = parse_time((const char *)(sqlite3_column_text(st_, col)));
		return true;
	}
	virtual bool is_null(int col) {
		return do_is_null(col);
	}
	virtual int cols() {
		return cols_;
	}
	virtual int name_to_column(const std::string &n) {
		if (!column_names_prepared_) {
			for (int i = 0; i < cols_; i++) {
				const char *name = sqlite3_column_name(st_, i);
				if (!name) {
					throw std::bad_alloc();
				}
				column_names_[name] = i;
			}
			column_names_prepared_ = true;
		}
		std::map<std::string, int>::const_iterator p = column_names_.find(n);
		if (p == column_names_.end())
			return -1;
		return p->second;
	}
	virtual std::string column_to_name(int col) {
		check(col);
		const char *name = sqlite3_column_name(st_, col);
		if (!name) {
			throw std::bad_alloc();
		}
		return name;
	}

private:
	bool do_is_null(int col) {
		check(col);
		return sqlite3_column_type(st_, col) == SQLITE_NULL;
	}
	void check(int col) {
		if (col < 0 || col >= cols_)
			throw invalid_column();
	}
	sqlite3_stmt *st_;
	sqlite3 *conn_;
	std::map<std::string, int> column_names_;
	bool column_names_prepared_;
	int cols_;
};

class statement : public backend::statement {
public:
	virtual void reset() {
		reset_stat();
		sqlite3_clear_bindings(st_);
	}
	void reset_stat() {
		if (!reset_) {
			sqlite3_reset(st_);
			reset_ = true;
		}
	}
	virtual void bind(int col, const std::string &v) {
		reset_stat();
		check_bind(sqlite3_bind_text(st_, col, v.c_str(), v.size(), SQLITE_TRANSIENT));
	}
	virtual void bind(int col, const char *s) {
		reset_stat();
		check_bind(sqlite3_bind_text(st_, col, s, -1, SQLITE_TRANSIENT));
	}
	virtual void bind(int col, const char *b, const char *e) {
		reset_stat();
		check_bind(sqlite3_bind_text(st_, col, b, e - b, SQLITE_TRANSIENT));
	}
	virtual void bind(int col, const std::tm &v) {
		reset_stat();
		std::string tmp = cppdb::format_time(v);
		check_bind(sqlite3_bind_text(st_, col, tmp.c_str(), tmp.size(), SQLITE_TRANSIENT));
	}
	virtual void bind(int col, std::istream &v) {
		reset_stat();
		// TODO Fix me
		std::ostringstream ss;
		ss << v.rdbuf();
		std::string tmp = ss.str();
		check_bind(sqlite3_bind_text(st_, col, tmp.c_str(), tmp.size(), SQLITE_TRANSIENT));
	}
	virtual void bind(int col, int v) {
		reset_stat();
		check_bind(sqlite3_bind_int(st_, col, v));
	}
	template <typename IntType>
	void do_bind(int col, IntType value) {
		reset_stat();
		int r;
		if (sizeof(value) > sizeof(int) || (long long)(value) > std::numeric_limits<int>::max())
			r = sqlite3_bind_int64(st_, col, static_cast<sqlite3_int64>(value));
		else
			r = sqlite3_bind_int(st_, col, static_cast<int>(value));
		check_bind(r);
	}
	virtual void bind(int col, unsigned v) {
		do_bind(col, v);
	}
	virtual void bind(int col, long v) {
		do_bind(col, v);
	}
	virtual void bind(int col, unsigned long v) {
		do_bind(col, v);
	}
	virtual void bind(int col, long long v) {
		do_bind(col, v);
	}
	virtual void bind(int col, unsigned long long v) {
		do_bind(col, v);
	}
	virtual void bind(int col, double v) {
		reset_stat();
		check_bind(sqlite3_bind_double(st_, col, v));
	}
	virtual void bind(int col, long double v) {
		reset_stat();
		check_bind(sqlite3_bind_double(st_, col, static_cast<double>(v)));
	}
	virtual void bind_null(int col) {
		reset_stat();
		check_bind(sqlite3_bind_null(st_, col));
	}
	virtual std::shared_ptr<backend::result> query() {
		reset_stat();
		reset_ = false;
		return std::make_shared<result>(st_, conn_);
	}
	virtual long long sequence_last(const std::string & /*name*/) {
		return sqlite3_last_insert_rowid(conn_);
	}
	virtual void exec() {
		reset_stat();
		reset_ = false;
		int r = sqlite3_step(st_);
		if (r != SQLITE_DONE) {
			if (r == SQLITE_ROW) {
				throw cppdb_error("Using exec with query!");
			} else
				check_bind(r);
		}
	}
	virtual unsigned long long affected() {
		return sqlite3_changes(conn_);
	}
	virtual const std::string &sql_query() {
		return sql_query_;
	}
	statement(const std::string &query, sqlite3 *conn) : st_(0), conn_(conn), reset_(true), sql_query_(query) {
		if (sqlite3_prepare_v2(conn_, query.c_str(), query.size(), &st_, 0) != SQLITE_OK)
			throw cppdb_error(sqlite3_errmsg(conn_));
	}
	~statement() {
		sqlite3_finalize(st_);
	}

private:
	void check_bind(int v) {
		if (v == SQLITE_RANGE) {
			throw invalid_placeholder();
		}
		if (v != SQLITE_OK) {
			throw cppdb_error(sqlite3_errmsg(conn_));
		}
	}
	sqlite3_stmt *st_;
	sqlite3 *conn_;
	bool reset_;
	std::string sql_query_;
};
class connection : public backend::connection {
public:
	connection(const connection_info &ci) : backend::connection(ci), conn_(0) {
		std::string dbname = ci.get("db");
		if (dbname.empty()) {
			throw cppdb_error("sqlite3:database file (db propery) not specified");
		}

		std::string mode = ci.get("mode", "create");
		int flags = 0;
		if (mode == "create")
			flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
		else if (mode == "readonly")
			flags = SQLITE_OPEN_READONLY;
		else if (mode == "readwrite")
			flags = SQLITE_OPEN_READWRITE;
		else {
			throw cppdb_error(
				"sqlite3:invalid mode propery, expected "
				" 'create' (default), 'readwrite' or 'readonly' values");
		}

		std::string vfs = ci.get("vfs");
		const char *cvfs = vfs.empty() ? (const char *)(0) : vfs.c_str();

		int busy = ci.get("busy_timeout", -1);

		try {
			if (sqlite3_open_v2(dbname.c_str(), &conn_, flags, cvfs) != SQLITE_OK) {
				if (conn_ == 0) {
					throw cppdb_error("sqlite3:failed to create db object");
				}

				throw cppdb_error(std::string("sqlite3:Failed to open connection:") + sqlite3_errmsg(conn_));
			}

			if (busy != -1 && sqlite3_busy_timeout(conn_, busy) != 0)
				throw cppdb_error(std::string("sqlite3:Failed to set timeout:") + sqlite3_errmsg(conn_));
		} catch (...) {
			if (conn_) {
				sqlite3_close(conn_);
				conn_ = 0;
			}
			throw;
		}
	}
	virtual ~connection() {
		sqlite3_close(conn_);
	}
	virtual void begin() {
		fast_exec("begin");
	}
	virtual void commit() {
		fast_exec("commit");
	}
	virtual void rollback() {
		fast_exec("rollback");
	}
	virtual statement *prepare_statement(const std::string &q) {
		return new statement(q, conn_);
	}
	virtual statement *create_statement(const std::string &q) {
		return prepare_statement(q);
	}
	virtual std::string escape(const std::string &s) {
		return escape(s.c_str(), s.c_str() + s.size());
	}
	virtual std::string escape(const char *s) {
		return escape(s, s + strlen(s));
	}
	virtual std::string escape(const char *b, const char *e) {
		std::string result;
		result.reserve(e - b);
		for (; b != e; b++) {
			char c = *b;
			if (c == '\'')
				result += "''";
			else
				result += c;
		}
		return result;
	}
	virtual std::string driver() {
		return "sqlite3";
	}
	virtual std::string engine() {
		return "sqlite3";
	}

private:
	void fast_exec(const char *query) {
		if (sqlite3_exec(conn_, query, 0, 0, 0) != SQLITE_OK) {
			throw cppdb_error(std::string("sqlite3:") + sqlite3_errmsg(conn_));
		}
	}

	sqlite3 *conn_;
};

} // namespace sqlite3_backend
} // namespace cppdb

extern "C" {
CPPDB_DRIVER_API cppdb::backend::connection *cppdb_sqlite3_get_connection(const cppdb::connection_info &cs) {
	return new cppdb::sqlite3_backend::connection(cs);
}
}
