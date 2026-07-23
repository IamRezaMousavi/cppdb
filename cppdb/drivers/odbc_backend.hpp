#include <cppdb/backend.hpp>
#include <cppdb/driver_manager.hpp>
#include <cppdb/numeric_util.hpp>
#include <cppdb/utils.hpp>

#include <sqlext.h>
#include <string.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <sstream>
#include <vector>

#if defined(_WIN32) || defined(__WIN32) || defined(WIN32) || defined(__CYGWIN__)
#include <windows.h>
#endif

namespace cppdb {
namespace odbc_backend {

typedef unsigned odbc_u32;
typedef unsigned short odbc_u16;

int assert_on_unsigned_is_32[sizeof(unsigned) == 4 ? 1 : -1];
int assert_on_unsigned_short_is_16[sizeof(unsigned short) == 2 ? 1 : -1];
int assert_on_sqlwchar_is_16[sizeof(SQLWCHAR) == 2 ? 1 : -1];

namespace utf {
static const odbc_u32 illegal = 0xFFFFFFFFu;
inline bool valid(odbc_u32 v) {
	if (v > 0x10FFFF)
		return false;
	if (0xD800 <= v && v <= 0xDFFF) // surragates
		return false;
	return true;
}
} // namespace utf

namespace utf8 {
// See RFC 3629
// Based on: http://www.w3.org/International/questions/qa-forms-utf-8
template <typename Iterator>
odbc_u32 next(Iterator &p, Iterator e) {
	unsigned char c = *p++;
	unsigned char seq0, seq1 = 0, seq2 = 0, seq3 = 0;
	seq0 = c;
	int len = 1;
	if ((c & 0xC0) == 0xC0) {
		if (p == e)
			return utf::illegal;
		seq1 = *p++;
		len = 2;
	}
	if ((c & 0xE0) == 0xE0) {
		if (p == e)
			return utf::illegal;
		seq2 = *p++;
		len = 3;
	}
	if ((c & 0xF0) == 0xF0) {
		if (p == e)
			return utf::illegal;
		seq3 = *p++;
		len = 4;
	}
	switch (len) {
	case 1: // ASCII -- remove codes for HTML only
		if (seq0 > 0x7F)
			return utf::illegal;
		break;
	case 2: // non-overloading 2 bytes
		if (0xC2 <= seq0 && seq0 <= 0xDF) {
			if (0x80 <= seq1 && seq1 <= 0xBF)
				break;
		}
		return utf::illegal;
	case 3:
		if (seq0 == 0xE0) { // exclude overloadings
			if (0xA0 <= seq1 && seq1 <= 0xBF && 0x80 <= seq2 && seq2 <= 0xBF)
				break;
		} else if ((0xE1 <= seq0 && seq0 <= 0xEC) || seq0 == 0xEE || seq0 == 0xEF) { // stright 3 bytes
			if (0x80 <= seq1 && seq1 <= 0xBF && 0x80 <= seq2 && seq2 <= 0xBF)
				break;
		} else if (seq0 == 0xED) { // exclude surrogates
			if (0x80 <= seq1 && seq1 <= 0x9F && 0x80 <= seq2 && seq2 <= 0xBF)
				break;
		}
		return utf::illegal;
	case 4:
		switch (seq0) {
		case 0xF0: // planes 1-3
			if (0x90 <= seq1 && seq1 <= 0xBF && 0x80 <= seq2 && seq2 <= 0xBF && 0x80 <= seq3 && seq3 <= 0xBF)
				break;
			return utf::illegal;
		case 0xF1: // planes 4-15
		case 0xF2:
		case 0xF3:
			if (0x80 <= seq1 && seq1 <= 0xBF && 0x80 <= seq2 && seq2 <= 0xBF && 0x80 <= seq3 && seq3 <= 0xBF)
				break;
			return utf::illegal;
		case 0xF4: // pane 16
			if (0x80 <= seq1 && seq1 <= 0x8F && 0x80 <= seq2 && seq2 <= 0xBF && 0x80 <= seq3 && seq3 <= 0xBF)
				break;
			return utf::illegal;
		default:
			return utf::illegal;
		}
	}

	switch (len) {
	case 1:
		return seq0;
	case 2:
		return ((seq0 & 0x1F) << 6) | (seq1 & 0x3F);
	case 3:
		return ((seq0 & 0x0F) << 12) | ((seq1 & 0x3F) << 6) | (seq2 & 0x3F);
	case 4:
		return ((seq0 & 0x07) << 18) | ((seq1 & 0x3F) << 12) | ((seq2 & 0x3F) << 6) | (seq3 & 0x3F);
	}

	return utf::illegal;
} // valid

struct seq {
	char c[4];
	unsigned len;
};
inline seq encode(odbc_u32 value) {
	seq out = seq();
	if (value <= 0x7F) {
		out.c[0] = value;
		out.len = 1;
	} else if (value <= 0x7FF) {
		out.c[0] = (value >> 6) | 0xC0;
		out.c[1] = (value & 0x3F) | 0x80;
		out.len = 2;
	} else if (value <= 0xFFFF) {
		out.c[0] = (value >> 12) | 0xE0;
		out.c[1] = ((value >> 6) & 0x3F) | 0x80;
		out.c[2] = (value & 0x3F) | 0x80;
		out.len = 3;
	} else {
		out.c[0] = (value >> 18) | 0xF0;
		out.c[1] = ((value >> 12) & 0x3F) | 0x80;
		out.c[2] = ((value >> 6) & 0x3F) | 0x80;
		out.c[3] = (value & 0x3F) | 0x80;
		out.len = 4;
	}
	return out;
}
} // namespace utf8

namespace utf16 {

// See RFC 2781
inline bool is_first_surrogate(odbc_u16 x) {
	return 0xD800 <= x && x <= 0xDBFF;
}
inline bool is_second_surrogate(odbc_u16 x) {
	return 0xDC00 <= x && x <= 0xDFFF;
}
inline odbc_u32 combine_surrogate(odbc_u16 w1, odbc_u16 w2) {
	return ((odbc_u32(w1 & 0x3FF) << 10) | (w2 & 0x3FF)) + 0x10000;
}

template <typename It>
inline odbc_u32 next(It &current, It last) {
	odbc_u16 w1 = *current++;
	if (w1 < 0xD800 || 0xDFFF < w1) {
		return w1;
	}
	if (w1 > 0xDBFF)
		return utf::illegal;
	if (current == last)
		return utf::illegal;
	odbc_u16 w2 = *current++;
	if (w2 < 0xDC00 || 0xDFFF < w2)
		return utf::illegal;
	return combine_surrogate(w1, w2);
}
inline int width(odbc_u32 u) {
	return u >= 0x100000 ? 2 : 1;
}
struct seq {
	odbc_u16 c[2];
	unsigned len;
};
inline seq encode(odbc_u32 u) {
	seq out = seq();
	if (u <= 0xFFFF) {
		out.c[0] = u;
		out.len = 1;
	} else {
		u -= 0x10000;
		out.c[0] = 0xD800 | (u >> 10);
		out.c[1] = 0xDC00 | (u & 0x3FF);
		out.len = 2;
	}
	return out;
}
} // namespace utf16

} // namespace odbc_backend
} // namespace cppdb

namespace cppdb {

class connection_info;
class pool;

namespace odbc_backend {

std::string widen(const char *b, const char *e) {
	std::string result;
	result.reserve((e - b) * 2);
	odbc_u32 code_point = 0;
	while (b < e && (code_point = utf8::next(b, e)) != utf::illegal) {
		utf16::seq sq = utf16::encode(code_point);
		char *str = (char *)sq.c;
		result.append(str, sq.len * 2);
	}
	if (b != e || code_point == utf::illegal) {
		throw cppdb_error("cppdb::odbc invalid UTF-8 input");
	}
	return result;
}

std::string widen(const std::string &s) {
	return widen(s.c_str(), s.c_str() + s.size());
}

std::string narrower(const std::basic_string<SQLWCHAR> &wide) {
	const odbc_u16 *b = reinterpret_cast<const odbc_u16 *>(wide.c_str());
	const odbc_u16 *e = b + wide.size();

	std::string result;
	result.reserve((e - b));

	odbc_u32 code_point = 0;
	while (b < e && (code_point = utf16::next(b, e)) != utf::illegal) {
		utf8::seq sq = utf8::encode(code_point);
		result.append(sq.c, sq.len);
	}
	if (b != e || code_point == utf::illegal) {
		throw cppdb_error("cppdb::odbc got invalid UTF-16");
	}
	return result;
}

std::string narrower(const std::string &wide) {
	if (wide.size() % 2 != 0) {
		throw cppdb_error("cppdb::odbc got invalid UTF-16");
	}
	const odbc_u16 *b = reinterpret_cast<const odbc_u16 *>(wide.c_str());
	const odbc_u16 *e = b + wide.size() / 2;

	std::string result;
	result.reserve((e - b));

	odbc_u32 code_point = 0;
	while (b < e && (code_point = utf16::next(b, e)) != utf::illegal) {
		utf8::seq sq = utf8::encode(code_point);
		result.append(sq.c, sq.len);
	}
	if (b != e || code_point == utf::illegal) {
		throw cppdb_error("cppdb::odbc got invalid UTF-16");
	}
	return result;
}

std::basic_string<SQLWCHAR> tosqlwide(const std::string &n) {
	std::basic_string<SQLWCHAR> result;
	const char *b = n.c_str();
	const char *e = b + n.size();
	result.reserve(e - b);
	odbc_u32 code_point = 0;
	while (b < e && (code_point = utf8::next(b, e)) != utf::illegal) {
		utf16::seq sq = utf16::encode(code_point);
		result.append((SQLWCHAR *)sq.c, sq.len);
	}
	if (b != e || code_point == utf::illegal) {
		throw cppdb_error("cppdb::odbc invalid UTF-8 input");
	}
	return result;
}

void check_odbc_errorW(SQLRETURN error, SQLHANDLE h, SQLSMALLINT type) {
	if (SQL_SUCCEEDED(error))
		return;
	std::basic_string<SQLWCHAR> error_message;
	int rec = 1;
	for (;;) {
		SQLWCHAR msg[SQL_MAX_MESSAGE_LENGTH + 2] = {0};
		SQLWCHAR stat[SQL_SQLSTATE_SIZE + 1] = {0};
		SQLINTEGER err;
		SQLSMALLINT len;
		SQLRETURN r = SQLGetDiagRecW(type, h, rec, stat, &err, msg, sizeof(msg) / sizeof(SQLWCHAR), &len);
		rec++;
		if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
			if (!error_message.empty()) {
				SQLWCHAR nl = '\n';
				error_message += nl;
			}
			error_message.append(msg);
		} else
			break;
	}
	std::string utf8_str = "Unconvertable string";
	try {
		std::string tmp = narrower(error_message);
		utf8_str = tmp;
	} catch (...) {}
	throw cppdb_error("cppdb::odbc_backend::Failed with error `" + utf8_str + "'");
}

void check_odbc_errorA(SQLRETURN error, SQLHANDLE h, SQLSMALLINT type) {
	if (SQL_SUCCEEDED(error))
		return;
	std::string error_message;
	int rec = 1;
	for (;;) {
		SQLCHAR msg[SQL_MAX_MESSAGE_LENGTH + 2] = {0};
		SQLCHAR stat[SQL_SQLSTATE_SIZE + 1] = {0};
		SQLINTEGER err;
		SQLSMALLINT len;
		SQLRETURN r = SQLGetDiagRecA(type, h, rec, stat, &err, msg, sizeof(msg), &len);
		rec++;
		if (r == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
			if (!error_message.empty())
				error_message += '\n';
			error_message += (char *)msg;
		} else
			break;
	}
	throw cppdb_error("cppdb::odbc::Failed with error `" + error_message + "'");
}

void check_odbc_error(SQLRETURN error, SQLHANDLE h, SQLSMALLINT type, bool wide) {
	if (wide)
		check_odbc_errorW(error, h, type);
	else
		check_odbc_errorA(error, h, type);
}

class result : public backend::result {
public:
	typedef std::pair<bool, std::string> cell_type;
	typedef std::vector<cell_type> row_type;
	typedef std::list<row_type> rows_type;

	next_row has_next() override {
		rows_type::iterator p = current_;
		if (p == rows_.end() || ++p == rows_.end())
			return last_row_reached;
		else
			return next_row_exists;
	}
	bool next() override {
		if (started_ == false) {
			current_ = rows_.begin();
			started_ = true;
		} else if (current_ != rows_.end()) {
			++current_;
		}
		return current_ != rows_.end();
	}
	template <typename T>
	bool do_fetch(int col, T &v) {
		if (at(col).first)
			return false;
		v = parse_number<T>(at(col).second, ss_);
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
		if (at(col).first)
			return false;
		v = at(col).second;
		return true;
	}
	bool fetch(int col, std::ostream &v) override {
		if (at(col).first)
			return false;
		v << at(col).second;
		return true;
	}
	bool fetch(int col, std::tm &v) override {
		if (at(col).first)
			return false;
		v = parse_time(at(col).second);
		return true;
	}
	bool is_null(int col) override {
		return at(col).first;
	}
	int cols() override {
		return cols_;
	}
	int name_to_column(const std::string &cn) override {
		for (size_t i = 0; i < names_.size(); i++)
			if (names_[i] == cn)
				return i;
		return -1;
	}
	std::string column_to_name(int c) override {
		if (c < 0 || c >= int(names_.size()))
			throw invalid_column();
		return names_[c];
	}

	result(rows_type &rows, std::vector<std::string> &names, int cols) : cols_(cols) {
		names_.swap(names);
		rows_.swap(rows);
		current_ = rows_.end();
		ss_.imbue(std::locale::classic());
	}
	cell_type &at(int col) {
		if (current_ != rows_.end() && col >= 0 && col < int(current_->size()))
			return current_->at(col);
		throw invalid_column();
	}

private:
	int cols_;
	bool started_ = false;
	std::vector<std::string> names_;
	rows_type::iterator current_;
	rows_type rows_;
	std::istringstream ss_;
};

class statements_cache;

class connection;

class statement : public backend::statement {
	struct parameter {
		void set_binary(const char *b, const char *e) {
			value.assign(b, e - b);
			null = false;
			ctype = SQL_C_BINARY;
			sqltype = SQL_LONGVARBINARY;
		}
		void set_text(const char *b, const char *e, bool wide) {
			if (!wide) {
				value.assign(b, e - b);
				null = false;
				ctype = SQL_C_CHAR;
				sqltype = SQL_LONGVARCHAR;
			} else {
				std::string tmp = widen(b, e);
				value.swap(tmp);
				null = false;
				ctype = SQL_C_WCHAR;
				sqltype = SQL_WLONGVARCHAR;
			}
		}
		void set(const std::tm &v) {
			value = cppdb::format_time(v);
			null = false;
			sqltype = SQL_C_TIMESTAMP;
			ctype = SQL_C_CHAR;
		}

		template <typename T>
		void set(T v) {
			std::ostringstream ss;
			ss.imbue(std::locale::classic());
			if (!std::numeric_limits<T>::is_integer)
				ss << std::setprecision(std::numeric_limits<T>::digits10 + 1);
			ss << v;

			value = ss.str();
			null = false;
			ctype = SQL_C_CHAR;
			if (std::numeric_limits<T>::is_integer)
				sqltype = SQL_INTEGER;
			else
				sqltype = SQL_DOUBLE;
		}
		void bind(int col, SQLHSTMT stmt, bool wide) {
			SQLRETURN r = 0;
			if (null) {
				lenval = SQL_NULL_DATA;
				r = SQLBindParameter(stmt, col, SQL_PARAM_INPUT, SQL_C_CHAR,
									 SQL_NUMERIC, // for null
									 10,		  // COLUMNSIZE
									 0,			  //  Presision
									 0,			  // string
									 0,			  // size
									 &lenval);
			} else {
				lenval = value.size();
				size_t column_size = value.size();
				if (ctype == SQL_C_WCHAR)
					column_size /= 2;
				if (value.empty())
					column_size = 1;
				r = SQLBindParameter(stmt, col, SQL_PARAM_INPUT, ctype, sqltype,
									 column_size,			// COLUMNSIZE
									 0,						//  Presision
									 (void *)value.c_str(), // string
									 value.size(), &lenval);
			}
			check_odbc_error(r, stmt, SQL_HANDLE_STMT, wide);
		}

		std::string value;
		bool null = true;
		SQLSMALLINT ctype = SQL_C_CHAR;
		SQLSMALLINT sqltype = SQL_C_NUMERIC;
		SQLLEN lenval = 0;
	};

public:
	// Begin of API
	void reset() override {
		SQLFreeStmt(stmt_, SQL_UNBIND);
		SQLCloseCursor(stmt_);
		params_.resize(0);
		if (params_no_ > 0)
			params_.resize(params_no_);
	}
	parameter &param_at(int col) {
		col--;
		if (col < 0)
			throw invalid_placeholder();
		if (params_no_ < 0) {
			if (params_.size() < size_t(col + 1))
				params_.resize(col + 1);
		} else if (col >= params_no_) {
			throw invalid_placeholder();
		}
		return params_[col];
	}
	const std::string &sql_query() override {
		return query_;
	}
	void bind(int col, const std::string &s) override {
		bind(col, s.c_str(), s.c_str() + s.size());
	}
	void bind(int col, const char *s) override {
		bind(col, s, s + strlen(s));
	}
	void bind(int col, const char *b, const char *e) override {
		param_at(col).set_text(b, e, wide_);
	}
	void bind(int col, const std::tm &s) override {
		param_at(col).set(s);
	}
	void bind(int col, std::istream &in) override {
		std::ostringstream ss;
		ss << in.rdbuf();
		std::string s = ss.str();
		param_at(col).set_binary(s.c_str(), s.c_str() + s.size());
	}
	template <typename T>
	void do_bind_num(int col, T v) {
		param_at(col).set(v);
	}
	void bind(int col, int v) override {
		do_bind_num(col, v);
	}
	void bind(int col, unsigned v) override {
		do_bind_num(col, v);
	}
	void bind(int col, long v) override {
		do_bind_num(col, v);
	}
	void bind(int col, unsigned long v) override {
		do_bind_num(col, v);
	}
	void bind(int col, long long v) override {
		do_bind_num(col, v);
	}
	void bind(int col, unsigned long long v) override {
		do_bind_num(col, v);
	}
	void bind(int col, double v) override {
		do_bind_num(col, v);
	}
	void bind(int col, long double v) override {
		do_bind_num(col, v);
	}
	void bind_null(int col) override {
		param_at(col) = parameter();
	}
	void bind_all() {
		for (unsigned i = 0; i < params_.size(); i++) {
			params_[i].bind(i + 1, stmt_, wide_);
		}
	}
	long long sequence_last(const std::string &sequence) override {
		std::shared_ptr<statement> st;
		if (!sequence_last_.empty()) {
			st = backend::make_stmt<statement>(sequence_last_, dbc_, wide_, false);
			st->bind(1, sequence);
		} else if (!last_insert_id_.empty()) {
			st = backend::make_stmt<statement>(last_insert_id_, dbc_, wide_, false);
		} else {
			throw not_supported_by_backend(
				"cppdb::odbc::sequence_last is not supported by odbc backend "
				"unless properties @squence_last, @last_insert_id are specified "
				"or @engine is one of mysql, sqlite3, postgresql, mssql");
		}
		auto res = st->query();
		long long last_id = 0LL;
		if (!res->next() || res->cols() != 1 || !res->fetch(0, last_id)) {
			throw cppdb_error("cppdb::odbc::sequence_last failed to fetch last value");
		}

		return last_id;
	}
	unsigned long long affected() override {
		SQLLEN rows = 0;
		SQLRETURN r = SQLRowCount(stmt_, &rows);
		check_error(r);
		return rows;
	}
	std::shared_ptr<backend::result> query() override {
		bind_all();
		SQLRETURN r = real_exec();
		check_error(r);
		result::rows_type rows;
		result::row_type row;

		std::string value;
		bool is_null = false;
		SQLSMALLINT ocols;
		r = SQLNumResultCols(stmt_, &ocols);
		check_error(r);
		int cols = ocols;

		std::vector<std::string> names(cols);
		std::vector<int> types(cols, SQL_C_CHAR);

		for (int col = 0; col < cols; col++) {
			SQLSMALLINT name_length = 0, data_type = 0, digits = 0, nullable = 0;
			SQLULEN collen = 0;

			if (wide_) {
				SQLWCHAR name[257] = {0};
				r = SQLDescribeColW(stmt_, col + 1, name, 256, &name_length, &data_type, &collen, &digits, &nullable);
				check_error(r);
				names[col] = narrower(name);
			} else {
				SQLCHAR name[257] = {0};
				r = SQLDescribeColA(stmt_, col + 1, name, 256, &name_length, &data_type, &collen, &digits, &nullable);
				check_error(r);
				names[col] = (char *)name;
			}
			switch (data_type) {
			case SQL_CHAR:
			case SQL_VARCHAR:
			case SQL_LONGVARCHAR:
				types[col] = SQL_C_CHAR;
				break;
			case SQL_WCHAR:
			case SQL_WVARCHAR:
			case SQL_WLONGVARCHAR:
				types[col] = SQL_C_WCHAR;
				break;
			case SQL_BINARY:
			case SQL_VARBINARY:
			case SQL_LONGVARBINARY:
				types[col] = SQL_C_BINARY;
				break;
			default:
				types[col] = SQL_C_DEFAULT;
				// Just a hack, actually I'm going to use C
				;
			}
		}

		while ((r = SQLFetch(stmt_)) == SQL_SUCCESS || r == SQL_SUCCESS_WITH_INFO) {
			row.resize(cols);
			for (int col = 0; col < cols; col++) {
				SQLLEN len = 0;
				is_null = false;
				int type = types[col];
				if (type == SQL_C_DEFAULT) {
					char buf[64];
					SQLRETURN r = SQLGetData(stmt_, col + 1, SQL_C_CHAR, buf, sizeof(buf), &len);
					check_error(r);
					if (len == SQL_NULL_DATA) {
						is_null = true;
					} else if (len <= 64) {
						value.assign(buf, len);
					} else {
						throw cppdb_error("cppdb::odbc::query - data too long");
					}
				} else {
					char buf[1024];
					size_t real_len = 0;
					if (type == SQL_C_CHAR) {
						real_len = sizeof(buf) - 1;
					} else if (type == SQL_C_BINARY) {
						real_len = sizeof(buf);
					} else { // SQL_C_WCHAR
						real_len = sizeof(buf) - sizeof(SQLWCHAR);
					}

					r = SQLGetData(stmt_, col + 1, type, buf, sizeof(buf), &len);
					check_error(r);
					if (len == SQL_NULL_DATA) {
						is_null = true;
					} else if (len == SQL_NO_TOTAL) {
						while (len == SQL_NO_TOTAL) {
							value.append(buf, real_len);
							r = SQLGetData(stmt_, col + 1, type, buf, sizeof(buf), &len);
							check_error(r);
						}
						value.append(buf, len);
					} else if (0 <= len && size_t(len) <= real_len) {
						value.assign(buf, len);
					} else if (len >= 0) {
						value.assign(buf, real_len);
						size_t rem_len = len - real_len;
						std::vector<char> tmp(rem_len + 2, 0);
						r = SQLGetData(stmt_, col + 1, type, &tmp[0], tmp.size(), &len);
						check_error(r);
						value.append(&tmp[0], rem_len);
					} else {
						throw cppdb_error("cppdb::odbc::query invalid result length");
					}
					if (!is_null && type == SQL_C_WCHAR) {
						std::string tmp = narrower(value);
						value.swap(tmp);
					}
				}

				row[col].first = is_null;
				row[col].second.swap(value);
			}
			rows.push_back(result::row_type());
			rows.back().swap(row);
		}
		if (r != SQL_NO_DATA) {
			check_error(r);
		}
		return std::make_shared<result>(rows, names, cols);
	}

	SQLRETURN real_exec() {
		SQLRETURN r = 0;
		if (prepared_) {
			r = SQLExecute(stmt_);
		} else {
			if (wide_)
				r = SQLExecDirectW(stmt_, (SQLWCHAR *)tosqlwide(query_).c_str(), SQL_NTS);
			else
				r = SQLExecDirectA(stmt_, (SQLCHAR *)query_.c_str(), SQL_NTS);
		}
		return r;
	}
	void exec() override {
		bind_all();
		SQLRETURN r = real_exec();
		if (r != SQL_NO_DATA)
			check_error(r);
	}
	// End of API

	statement(const std::string &q, SQLHDBC dbc, bool wide, bool prepared)
		: dbc_(dbc), wide_(wide), query_(q), prepared_(prepared) {
		SQLRETURN r = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_);
		check_odbc_error(r, dbc, SQL_HANDLE_DBC, wide_);
		if (prepared_) {
			try {
				if (wide_) {
					r = SQLPrepareW(stmt_, (SQLWCHAR *)tosqlwide(query_).c_str(), SQL_NTS);
				} else {
					r = SQLPrepareA(stmt_, (SQLCHAR *)query_.c_str(), SQL_NTS);
				}
				check_error(r);
			} catch (...) {
				SQLFreeHandle(SQL_HANDLE_STMT, stmt_);
				throw;
			}
			SQLSMALLINT params_no;
			r = SQLNumParams(stmt_, &params_no);
			check_error(r);
			params_no_ = params_no;
			params_.resize(params_no_);
		} else {
			params_.reserve(50);
		}
	}
	~statement() override {
		SQLFreeHandle(SQL_HANDLE_STMT, stmt_);
	}

private:
	void check_error(SQLRETURN code) {
		check_odbc_error(code, stmt_, SQL_HANDLE_STMT, wide_);
	}

	SQLHDBC dbc_;
	SQLHSTMT stmt_;
	bool wide_;
	std::string query_;
	std::vector<parameter> params_;
	int params_no_ = -1;

	friend class connection;
	std::string sequence_last_;
	std::string last_insert_id_;
	bool prepared_;
};

class connection : public backend::connection {
public:
	connection(const connection_info &ci) : backend::connection(ci), ci_(ci) {
		std::string utf_mode = ci.get("@utf", "narrow");

		if (utf_mode == "narrow")
			wide_ = false;
		else if (utf_mode == "wide")
			wide_ = true;
		else
			throw cppdb_error("cppdb::odbc:: @utf property can be either 'narrow' or 'wide'");

		bool env_created = false;
		bool dbc_created = false;
		bool dbc_connected = false;

		try {
			SQLRETURN r = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
			if (!SQL_SUCCEEDED(r)) {
				throw cppdb_error("cppdb::odbc::Failed to allocate environment handle");
			}
			env_created = true;
			r = SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
			check_odbc_error(r, env_, SQL_HANDLE_ENV, wide_);
			r = SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_);
			check_odbc_error(r, env_, SQL_HANDLE_ENV, wide_);
			dbc_created = true;
			if (wide_) {
				r = SQLDriverConnectW(dbc_, 0, (SQLWCHAR *)tosqlwide(conn_str(ci)).c_str(), SQL_NTS, 0, 0, 0,
									  SQL_DRIVER_COMPLETE);
			} else {
				r = SQLDriverConnectA(dbc_, 0, (SQLCHAR *)conn_str(ci).c_str(), SQL_NTS, 0, 0, 0, SQL_DRIVER_COMPLETE);
			}
			check_odbc_error(r, dbc_, SQL_HANDLE_DBC, wide_);
		} catch (...) {
			if (dbc_connected)
				SQLDisconnect(dbc_);
			if (dbc_created)
				SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
			if (env_created)
				SQLFreeHandle(SQL_HANDLE_ENV, env_);
			throw;
		}
	}

	std::string conn_str(const connection_info &ci) {
		std::string str;
		for (const auto &p : ci.properties) {
			if (p.first.empty() || p.first[0] == '@')
				continue;
			str += p.first;
			str += "=";
			str += p.second;
			str += ";";
		}
		return str;
	}

	~connection() override {
		SQLDisconnect(dbc_);
		SQLFreeHandle(SQL_HANDLE_DBC, dbc_);
		SQLFreeHandle(SQL_HANDLE_ENV, env_);
	}

	/// API
	void begin() override {
		set_autocommit(false);
	}
	void commit() override {
		SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, dbc_, SQL_COMMIT);
		check_odbc_error(r, dbc_, SQL_HANDLE_DBC, wide_);
		set_autocommit(true);
	}

	void rollback() override {
		try {
			SQLRETURN r = SQLEndTran(SQL_HANDLE_DBC, dbc_, SQL_ROLLBACK);
			check_odbc_error(r, dbc_, SQL_HANDLE_DBC, wide_);
		} catch (...) {}
		try {
			set_autocommit(true);
		} catch (...) {}
	}
	std::shared_ptr<statement> real_prepare(const std::string &q, bool prepared) {
		auto st = backend::make_stmt<statement>(q, dbc_, wide_, prepared);
		std::string seq = ci_.get("@sequence_last", "");
		if (seq.empty()) {
			std::string eng = engine();
			if (eng == "sqlite3")
				st->last_insert_id_ = "select last_insert_rowid()";
			else if (eng == "mysql")
				st->last_insert_id_ = "select last_insert_id()";
			else if (eng == "postgresql")
				st->sequence_last_ = "select currval(?)";
			else if (eng == "mssql")
				st->last_insert_id_ = "select @@identity";
		} else {
			if (seq.find('?') == std::string::npos)
				st->last_insert_id_ = seq;
			else
				st->sequence_last_ = seq;
		}

		return st;
	}

	std::shared_ptr<backend::statement> prepare_statement(const std::string &q) override {
		return real_prepare(q, true);
	}

	std::shared_ptr<backend::statement> create_statement(const std::string &q) override {
		return real_prepare(q, false);
	}

	std::string escape(const std::string &s) override {
		return escape(s.c_str(), s.c_str() + s.size());
	}
	std::string escape(const char *s) override {
		return escape(s, s + strlen(s));
	}
	std::string escape(const char * /*b*/, const char * /*e*/) override {
		throw not_supported_by_backend("cppcms::odbc:: string escaping is not supported");
	}
	std::string driver() override {
		return "odbc";
	}
	std::string engine() override {
		return ci_.get("@engine", "unknown");
	}

	void set_autocommit(bool on) {
		SQLPOINTER mode = (SQLPOINTER)(on ? SQL_AUTOCOMMIT_ON : SQL_AUTOCOMMIT_OFF);
		SQLRETURN r = SQLSetConnectAttr(dbc_,				 // handler
										SQL_ATTR_AUTOCOMMIT, // option
										mode,				 // value
										0);
		check_odbc_error(r, dbc_, SQL_HANDLE_DBC, wide_);
	}

private:
	SQLHENV env_;
	SQLHDBC dbc_;
	bool wide_;
	connection_info ci_;
};

class driver final : public backend::driver {
public:
	std::shared_ptr<backend::connection> open(const connection_info &ci) override {
		return backend::make_conn<connection>(ci);
	}
};

} // namespace odbc_backend

namespace {

struct register_odbc {
	register_odbc() {
		driver_manager::instance().install_driver("odbc", std::make_shared<odbc_backend::driver>());
	}
} reg_odbc;

} // namespace

} // namespace cppdb
