#include <cppdb/backend.hpp>
#include <cppdb/driver_manager.hpp>
#include <cppdb/errors.hpp>
#include <cppdb/numeric_util.hpp>
#include <cppdb/utils.hpp>
#include <libpq/libpq-fs.h>

#include <libpq-fe.h>
#include <stdlib.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <vector>

namespace cppdb {
namespace postgresql {

typedef enum {
	lo_type,
	bytea_type
} blob_type;

class pqerror : public cppdb_error {
public:
	pqerror(const char *msg) : cppdb_error(message(msg)) {}
	pqerror(PGresult *r, const char *msg) : cppdb_error(message(msg, r)) {}
	pqerror(PGconn *c, const char *msg) : cppdb_error(message(msg, c)) {}

	static std::string message(const char *msg) {
		return std::string("cppdb::posgresql: ") + msg;
	}
	static std::string message(const char *msg, PGresult *r) {
		std::string m = "cppdb::posgresql: ";
		m += msg;
		m += ": ";
		m += PQresultErrorMessage(r);
		return m;
	}
	static std::string message(const char *msg, PGconn *c) {
		std::string m = "cppdb::posgresql: ";
		m += msg;
		m += ": ";
		m += PQerrorMessage(c);
		return m;
	}
};

class result : public backend::result {
public:
	result(PGresult *res, PGconn *conn, blob_type b)
		: res_(res), conn_(conn), rows_(PQntuples(res)), cols_(PQnfields(res)), blob_(b) {
		ss_.imbue(std::locale::classic());
	}
	~result() override {
		PQclear(res_);
	}
	next_row has_next() override {
		if (current_ + 1 < rows_)
			return next_row_exists;
		else
			return last_row_reached;
	}
	bool next() override {
		current_++;
		if (current_ < rows_) {
			return true;
		}
		return false;
	}

	template <typename T>
	bool do_fetch(int col, T &v) {
		if (do_isnull(col))
			return false;
		std::string tmp(PQgetvalue(res_, current_, col), PQgetlength(res_, current_, col));
		v = parse_number<T>(tmp, ss_);
		return true;
	}
	bool fetch(int col, short &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, unsigned short &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, int &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, unsigned &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, long &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, unsigned long &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, long long &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, unsigned long long &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, float &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, double &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, long double &v) override {
		return do_fetch(col, v);
	}
	bool fetch(int col, std::string &v) override {
		if (do_isnull(col))
			return false;
		v.assign(PQgetvalue(res_, current_, col), PQgetlength(res_, current_, col));
		return true;
	}
	bool fetch(int col, std::ostream &v) override {
		if (do_isnull(col))
			return false;
		if (blob_ == bytea_type) {
			unsigned char *val = (unsigned char *)PQgetvalue(res_, current_, col);
			size_t len = 0;
			unsigned char *buf = PQunescapeBytea(val, &len);
			if (!buf) {
				throw bad_value_cast();
			}
			try {
				v.write((char *)buf, len);
			} catch (...) {
				PQfreemem(buf);
				throw;
			}
			PQfreemem(buf);
		} else { // oid
			Oid id = 0;
			fetch(col, id);
			if (id == 0) {
				throw pqerror("fetching large object failed, oid=0");
			}
			int fd = -1;
			try {
				fd = lo_open(conn_, id, INV_READ | INV_WRITE);
				if (fd < 0)
					throw pqerror(conn_, "Failed opening large object for read");
				char buf[4096];
				for (;;) {
					int n = lo_read(conn_, fd, buf, sizeof(buf));
					if (n < 0)
						throw pqerror(conn_, "Failed reading large object");
					if (n >= 0)
						v.write(buf, n);
					if (n < int(sizeof(buf)))
						break;
				}
				int r = lo_close(conn_, fd);
				fd = -1;
				if (r < 0)
					throw pqerror(conn_, "error on close of large object");
			} catch (...) {
				if (fd != -1)
					lo_close(conn_, fd);
				throw;
			}
		}
		return true;
	}
	bool fetch(int col, std::tm &v) override {
		if (do_isnull(col))
			return false;
		v = parse_time(PQgetvalue(res_, current_, col));
		return true;
	}
	bool is_null(int col) override {
		return do_isnull(col);
	}
	int cols() override {
		return cols_;
	}
	int name_to_column(const std::string &n) override {
		return PQfnumber(res_, n.c_str());
	}
	std::string column_to_name(int pos) override {
		const char *name = PQfname(res_, pos);
		if (!name)
			return std::string();
		return name;
	}

private:
	void check(int c) {
		if (c < 0 || c >= cols_)
			throw invalid_column();
	}
	bool do_isnull(int col) {
		check(col);
		return PQgetisnull(res_, current_, col);
	}
	PGresult *res_;
	PGconn *conn_;
	int rows_;
	int cols_;
	int current_ = -1;
	blob_type blob_;
	std::istringstream ss_;
};

class statement : public backend::statement {
public:
	typedef enum {
		null_param,
		text_param,
		binary_param
	} param_type;

	statement(PGconn *conn, const std::string &src_query, blob_type b, unsigned long long prepared_id)
		: conn_(conn), orig_query_(src_query), blob_(b) {
		fmt_.imbue(std::locale::classic());

		query_.reserve(src_query.size());
		bool inside_string = false;
		for (unsigned i = 0; i < src_query.size(); i++) {
			char c = src_query[i];
			if (c == '\'') {
				inside_string = !inside_string;
			}
			if (!inside_string && c == '?') {
				query_ += '$';
				params_++;
				fmt_ << params_;
				query_ += fmt_.str();
				fmt_.str(std::string());
				fmt_.clear();
			} else {
				query_ += c;
			}
		}
		reset();

		if (prepared_id > 0) {
			fmt_.str(std::string());
			fmt_.clear();
			fmt_ << "cppdb_psqlstmt_" << prepared_id;
			prepared_id_ = fmt_.str();
			fmt_.str(std::string());
			fmt_.clear();

			PGresult *r = PQprepare(conn_, prepared_id_.c_str(), query_.c_str(), 0, 0);
			try {
				if (!r) {
					throw pqerror("Failed to create prepared statement object!");
				}
				if (PQresultStatus(r) != PGRES_COMMAND_OK)
					throw pqerror(r, "statement preparation failed");
			} catch (...) {
				if (r)
					PQclear(r);
				throw;
			}
			PQclear(r);
		}
	}
	~statement() override {
		try {
			if (res_) {
				PQclear(res_);
				res_ = 0;
			}
			if (!prepared_id_.empty()) {
				std::string stmt = "DEALLOCATE " + prepared_id_;
				res_ = PQexec(conn_, stmt.c_str());
				if (res_) {
					PQclear(res_);
					res_ = 0;
				}
			}
		} catch (...) {}
	}
	void reset() override {
		if (res_) {
			PQclear(res_);
			res_ = 0;
		}
		std::vector<std::string> vals(params_);
		std::vector<size_t> lengths(params_, 0);
		std::vector<const char *> pvals(params_, 0);
		std::vector<param_type> flags(params_, null_param);
		params_values_.swap(vals);
		params_pvalues_.swap(pvals);
		params_plengths_.swap(lengths);
		params_set_.swap(flags);
	}
	void bind(int col, const std::string &v) override {
		bind(col, v.c_str(), v.c_str() + v.size());
	}
	void bind(int col, const char *s) override {
		bind(col, s, s + strlen(s));
	}
	void bind(int col, const char *b, const char *e) override {
		check(col);
		params_pvalues_[col - 1] = b;
		params_plengths_[col - 1] = e - b;
		params_set_[col - 1] = text_param;
	}
	void bind(int col, const std::tm &v) override {
		check(col);
		params_values_[col - 1] = cppdb::format_time(v);
		params_set_[col - 1] = text_param;
	}
	void bind(int col, std::istream &in) override {
		check(col);
		if (blob_ == bytea_type) {
			std::ostringstream ss;
			ss << in.rdbuf();
			params_values_[col - 1] = ss.str();
			params_set_[col - 1] = binary_param;
		} else {
			Oid id = 0;
			int fd = -1;
			try {
				id = lo_creat(conn_, INV_READ | INV_WRITE);
				if (id == 0)
					throw pqerror(conn_, "failed to create large object");
				fd = lo_open(conn_, id, INV_READ | INV_WRITE);
				if (fd < 0)
					throw pqerror(conn_, "failed to open large object for writing");
				char buf[4096];
				for (;;) {
					in.read(buf, sizeof(buf));
					int bytes_read = in.gcount();
					if (bytes_read > 0) {
						int n = lo_write(conn_, fd, buf, bytes_read);
						if (n < 0) {
							throw pqerror(conn_, "failed writing to large object");
						}
					}
					if (bytes_read < int(sizeof(buf)))
						break;
				}
				int r = lo_close(conn_, fd);
				fd = -1;
				if (r < 0)
					throw pqerror(conn_, "error closing large object after write");
				bind(col, id);
			} catch (...) {
				if (fd < -1)
					lo_close(conn_, fd);
				if (id != 0)
					lo_unlink(conn_, id);
				throw;
			}
		}
	}

	template <typename T>
	void do_bind(int col, T v) {
		check(col);
		fmt_.str(std::string());
		fmt_.clear();
		if (!std::numeric_limits<T>::is_integer)
			fmt_ << std::setprecision(std::numeric_limits<T>::digits10 + 1);
		fmt_ << v;
		params_values_[col - 1] = fmt_.str();
		params_set_[col - 1] = text_param;
		fmt_.str(std::string());
		fmt_.clear();
	}

	void bind(int col, int v) override {
		do_bind(col, v);
	}
	void bind(int col, unsigned v) override {
		do_bind(col, v);
	}
	void bind(int col, long v) override {
		do_bind(col, v);
	}
	void bind(int col, unsigned long v) override {
		do_bind(col, v);
	}
	void bind(int col, long long v) override {
		do_bind(col, v);
	}
	void bind(int col, unsigned long long v) override {
		do_bind(col, v);
	}
	void bind(int col, double v) override {
		do_bind(col, v);
	}
	void bind(int col, long double v) override {
		do_bind(col, v);
	}
	void bind_null(int col) override {
		check(col);
		params_set_[col - 1] = null_param;
		std::string tmp;
		params_values_[col - 1].swap(tmp);
	}

	void real_query() {
		const char *const *pvalues = 0;
		int *plengths = 0;
		int *pformats = 0;
		std::vector<const char *> values;
		std::vector<int> lengths;
		std::vector<int> formats;
		if (params_ > 0) {
			values.resize(params_, 0);
			lengths.resize(params_, 0);
			formats.resize(params_, 0);
			for (unsigned i = 0; i < params_; i++) {
				if (params_set_[i] != null_param) {
					if (params_pvalues_[i] != 0) {
						values[i] = params_pvalues_[i];
						lengths[i] = params_plengths_[i];
					} else {
						values[i] = params_values_[i].c_str();
						lengths[i] = params_values_[i].size();
					}
					if (params_set_[i] == binary_param) {
						formats[i] = 1;
					}
				}
			}
			pvalues = &values.front();
			plengths = &lengths.front();
			pformats = &formats.front();
		}
		if (res_) {
			PQclear(res_);
			res_ = 0;
		}
		if (prepared_id_.empty()) {
			res_ = PQexecParams(conn_, query_.c_str(), params_,
								0, // param types
								pvalues, plengths,
								pformats, // format - text
								0		  // result format - text
			);
		} else {
			res_ = PQexecPrepared(conn_, prepared_id_.c_str(), params_, pvalues, plengths,
								  pformats, // format - text
								  0			// result format - text
			);
		}
	}

	std::shared_ptr<backend::result> query() override {
		real_query();
		switch (PQresultStatus(res_)) {
		case PGRES_TUPLES_OK: {
			auto ptr = std::make_shared<result>(res_, conn_, blob_);
			res_ = 0;
			return ptr;
		} break;
		case PGRES_COMMAND_OK:
			throw pqerror("Statement used instread of query");
			break;
		default:
			throw pqerror(res_, "query execution failed ");
		}
	}
	void exec() override {
		real_query();
		switch (PQresultStatus(res_)) {
		case PGRES_TUPLES_OK:
			throw pqerror("Query used instread of statement");
			break;
		case PGRES_COMMAND_OK:
			break;
		default:
			throw pqerror(res_, "statement execution failed ");
		}
	}
	long long sequence_last(const std::string &sequence) override {
		PGresult *res = 0;
		long long rowid = 0;
		try {
			const char *const param_ptr = sequence.c_str();
			res = PQexecParams(conn_, "SELECT currval($1)",
							   1,		   // 1 param
							   0,		   // types
							   &param_ptr, // param values
							   0,		   // lengths
							   0,		   // formats
							   0		   // string format
			);
			if (PQresultStatus(res) != PGRES_TUPLES_OK) {
				throw pqerror(res, "failed to fetch last sequence id");
			}
			const char *val = PQgetvalue(res, 0, 0);
			if (!val || *val == 0)
				throw pqerror("Failed to get value for sequecne id");
			fmt_.str(val);
			fmt_.clear();
			fmt_ >> rowid;
			fmt_.str(std::string());
			fmt_.clear();
		} catch (...) {
			if (res)
				PQclear(res);
			throw;
		}
		PQclear(res);
		return rowid;
	}
	unsigned long long affected() override {
		if (res_) {
			const char *s = PQcmdTuples(res_);
			if (!s || !*s)
				return 0;
			unsigned long long rows = 0;
			fmt_.str(s);
			fmt_.clear();
			fmt_ >> rows;
			fmt_.str(std::string());
			fmt_.clear();
			return rows;
		}
		return 0;
	}
	const std::string &sql_query() override {
		return orig_query_;
	}

private:
	void check(int col) {
		if (col < 1 || col > int(params_))
			throw invalid_placeholder();
	}
	PGresult *res_ = nullptr;
	PGconn *conn_;

	std::string query_;
	std::string orig_query_;
	unsigned params_ = 0;
	std::vector<std::string> params_values_;
	std::vector<const char *> params_pvalues_;
	std::vector<size_t> params_plengths_;
	std::vector<param_type> params_set_;
	std::string prepared_id_;
	std::stringstream fmt_;
	blob_type blob_;
};

class connection : public backend::connection {
public:
	void do_simple_exec(const char *s) {
		PGresult *r = PQexec(conn_, s);
		try {
		} catch (...) {
			PQclear(r);
			throw;
		}
		PQclear(r);
	}
	void begin() override {
		do_simple_exec("begin");
	}
	void commit() override {
		do_simple_exec("commit");
	}
	void rollback() override {
		try {
			do_simple_exec("rollback");
		} catch (...) {}
	}
	std::shared_ptr<backend::statement> prepare_statement(const std::string &q) override {
		return backend::make_stmt<statement>(conn_, q, blob_, ++prepared_id_);
	}
	std::shared_ptr<backend::statement> create_statement(const std::string &q) override {
		return backend::make_stmt<statement>(conn_, q, blob_, 0);
	}
	std::string do_escape(const char *b, size_t length) {
		std::vector<char> buf(2 * length + 1);
		size_t len = PQescapeStringConn(conn_, &buf.front(), b, length, 0);
		return std::string(&buf.front(), len);
	}
	std::string escape(const std::string &s) override {
		return do_escape(s.c_str(), s.size());
	}
	std::string escape(const char *s) override {
		return do_escape(s, strlen(s));
	}
	std::string escape(const char *b, const char *e) override {
		return do_escape(b, e - b);
	}
	std::string pq_string(const connection_info &ci) {
		std::string pq_str;
		for (const auto &p : ci.properties) {
			if (p.first.empty() || p.first[0] == '@')
				continue;
			pq_str += p.first;
			pq_str += "='";
			pq_str += escape_for_conn(p.second);
			pq_str += "' ";
		}
		return pq_str;
	}
	std::string escape_for_conn(const std::string &v) {
		std::string res;
		res.reserve(v.size());
		for (unsigned i = 0; i < v.size(); i++) {
			if (v[i] == '\\')
				res += "\\\\";
			else if (v[i] == '\'')
				res += "\\\'";
			else
				res += v[i];
		}
		return res;
	}
	connection(const connection_info &ci) : backend::connection(ci) {
		std::string pq = pq_string(ci);
		std::string blob = ci.get("@blob", "lo");

		if (blob == "bytea")
			blob_ = bytea_type;
		else if (blob == "lo")
			blob_ = lo_type;
		else
			throw pqerror("@blob property should be either lo or bytea");

		conn_ = 0;
		try {
			conn_ = PQconnectdb(pq.c_str());
			if (!conn_)
				throw pqerror("failed to create connection object");
			if (PQstatus(conn_) != CONNECTION_OK)
				throw pqerror(conn_, "failed to connect");
		} catch (...) {
			if (conn_) {
				PQfinish(conn_);
				conn_ = 0;
			}
			throw;
		}
	}
	~connection() override {
		PQfinish(conn_);
	}
	std::string driver() override {
		return "postgresql";
	}
	std::string engine() override {
		return "postgresql";
	}

private:
	PGconn *conn_ = nullptr;
	unsigned long long prepared_id_ = 0;
	blob_type blob_;
};

class driver final : public backend::driver {
public:
	std::shared_ptr<backend::connection> open(const connection_info &ci) override {
		return backend::make_conn<connection>(ci);
	}
};

} // namespace postgresql

namespace {

struct register_postgresql {
	register_postgresql() {
		driver_manager::instance().install_driver("postgresql", std::make_shared<postgresql::driver>());
	}
} reg_postgresql;

} // namespace

} // namespace cppdb
