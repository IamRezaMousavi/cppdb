#include <cppdb/backend.hpp>
#include <cppdb/conn_manager.hpp>
#include <cppdb/frontend.hpp>
#include <cppdb/logging.hpp>
#include <cppdb/pool.hpp>

namespace cppdb {

class throw_guard {
public:
	throw_guard(const std::shared_ptr<backend::connection> &conn) : conn_(conn) {}
	void done() {
		conn_.reset();
	}
	~throw_guard() {
		if (conn_ && std::uncaught_exceptions()) {
			conn_->recyclable(false);
		}
	}

private:
	std::shared_ptr<backend::connection> conn_;
};

result::result(const std::shared_ptr<backend::result> &res, const std::shared_ptr<backend::statement> &stat,
			   const std::shared_ptr<backend::connection> &conn)
	: res_(res), stat_(stat), conn_(conn) {}

result::~result() {
	clear();
}

int result::cols() {
	return res_->cols();
}

bool result::next() {
	throw_guard g(conn_);

	if (eof_)
		return false;
	fetched_ = true;
	eof_ = res_->next() == false;
	current_col_ = 0;
	return !eof_;
}

int result::index(const std::string &n) {
	int c = res_->name_to_column(n);
	if (c < 0)
		throw invalid_column();
	return c;
}

std::string result::name(int col) {
	if (col < 0 || col >= cols())
		throw invalid_column();
	return res_->column_to_name(col);
}

int result::find_column(const std::string &name) {
	int c = res_->name_to_column(name);
	if (c < 0)
		return -1;
	return c;
}

void result::rewind_column() {
	current_col_ = 0;
}

bool result::empty() {
	if (!res_)
		return true;
	return eof_ || !fetched_;
}

void result::clear() {
	eof_ = true;
	fetched_ = true;
	res_.reset();
	stat_.reset();
	conn_.reset();
}

void result::check() {
	if (empty())
		throw empty_row_access();
}

bool result::is_null(int col) {
	return res_->is_null(col);
}
bool result::is_null(const std::string &n) {
	return is_null(index(n));
}

bool result::fetch(int col, short &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, unsigned short &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, int &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, unsigned &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, long &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, unsigned long &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, long long &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, unsigned long long &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, float &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, double &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, long double &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, std::string &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, std::tm &v) {
	return res_->fetch(col, v);
}
bool result::fetch(int col, std::ostream &v) {
	return res_->fetch(col, v);
}

bool result::fetch(const std::string &n, short &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, unsigned short &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, int &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, unsigned &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, long &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, unsigned long &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, long long &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, unsigned long long &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, float &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, double &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, long double &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, std::string &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, std::tm &v) {
	return res_->fetch(index(n), v);
}
bool result::fetch(const std::string &n, std::ostream &v) {
	return res_->fetch(index(n), v);
}

bool result::fetch(short &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(unsigned short &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(int &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(unsigned &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(long &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(unsigned long &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(long long &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(unsigned long long &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(float &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(double &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(long double &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(std::string &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(std::tm &v) {
	return res_->fetch(current_col_++, v);
}
bool result::fetch(std::ostream &v) {
	return res_->fetch(current_col_++, v);
}

statement::~statement() {
	stat_.reset();
	conn_.reset();
}

statement::statement(const std::shared_ptr<backend::statement> &stat, const std::shared_ptr<backend::connection> &conn)
	: stat_(stat), conn_(conn) {}

bool statement::empty() const {
	return !stat_;
}

void statement::clear() {
	stat_.reset();
	conn_.reset();
}

void statement::reset() {
	throw_guard g(conn_);
	placeholder_ = 1;
	stat_->reset();
}

statement &statement::operator<<(const std::string &v) {
	return bind(v);
}
statement &statement::operator<<(const char *s) {
	return bind(s);
}

statement &statement::operator<<(const std::tm &v) {
	return bind(v);
}

statement &statement::operator<<(std::istream &v) {
	return bind(v);
}

statement &statement::operator<<(void (*manipulator)(statement &st)) {
	manipulator(*this);
	return *this;
}
result statement::operator<<(result (*manipulator)(statement &st)) {
	return manipulator(*this);
}

statement &statement::bind(int v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(unsigned v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(long v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(unsigned long v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(long long v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(unsigned long long v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(double v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(long double v) {
	stat_->bind(placeholder_++, v);
	return *this;
}

statement &statement::bind(const std::string &v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(const char *s) {
	stat_->bind(placeholder_++, s);
	return *this;
}
statement &statement::bind(const char *b, const char *e) {
	stat_->bind(placeholder_++, b, e);
	return *this;
}
statement &statement::bind(const std::tm &v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind(std::istream &v) {
	stat_->bind(placeholder_++, v);
	return *this;
}
statement &statement::bind_null() {
	stat_->bind_null(placeholder_++);
	return *this;
}

void statement::bind(int col, const std::string &v) {
	stat_->bind(col, v);
}
void statement::bind(int col, const char *s) {
	stat_->bind(col, s);
}
void statement::bind(int col, const char *b, const char *e) {
	stat_->bind(col, b, e);
}
void statement::bind(int col, const std::tm &v) {
	stat_->bind(col, v);
}
void statement::bind(int col, std::istream &v) {
	stat_->bind(col, v);
}
void statement::bind(int col, int v) {
	stat_->bind(col, v);
}
void statement::bind(int col, unsigned v) {
	stat_->bind(col, v);
}
void statement::bind(int col, long v) {
	stat_->bind(col, v);
}
void statement::bind(int col, unsigned long v) {
	stat_->bind(col, v);
}
void statement::bind(int col, long long v) {
	stat_->bind(col, v);
}
void statement::bind(int col, unsigned long long v) {
	stat_->bind(col, v);
}
void statement::bind(int col, double v) {
	stat_->bind(col, v);
}
void statement::bind(int col, long double v) {
	stat_->bind(col, v);
}
void statement::bind_null(int col) {
	stat_->bind_null(col);
}

long long statement::last_insert_id() {
	throw_guard g(conn_);
	return stat_->sequence_last(std::string());
}

long long statement::sequence_last(const std::string &seq) {
	throw_guard g(conn_);
	return stat_->sequence_last(seq);
}
unsigned long long statement::affected() {
	throw_guard g(conn_);
	return stat_->affected();
}

result statement::row() {
	CPPDB_LOG_INFO << "statement: fetch one row";
	throw_guard g(conn_);
	auto backend_res = stat_->query();
	result res(backend_res, stat_, conn_);
	if (res.next()) {
		if (res.res_->has_next() == backend::result::next_row_exists) {
			g.done();
			// throw multiple_rows_query();
			CPPDB_LOG_WARNING << "statement: fetch more than one row with row operation";
		}
	}
	return res;
}

result statement::query() {
	CPPDB_LOG_INFO << "statement: fetch query with multi row";
	throw_guard g(conn_);
	auto res(stat_->query());
	return result(res, stat_, conn_);
}
statement::operator result() {
	return query();
}
void statement::exec() {
	CPPDB_LOG_INFO << "statement: exec";
	throw_guard g(conn_);
	stat_->exec();
}

session::session(const std::shared_ptr<backend::connection> &conn) : conn_(conn) {}
session::session(const std::shared_ptr<backend::connection> &conn, const once_functor &f) : conn_(conn) {
	once(f);
}
session::~session() {
	CPPDB_LOG_DEBUG << "session: destroy";
}
session::session(const connection_info &ci) {
	open(ci);
}
session::session(const std::string &cs) {
	open(cs);
}
session::session(const connection_info &ci, const once_functor &f) {
	open(ci);
	once(f);
}
session::session(const std::string &cs, const once_functor &f) {
	open(cs);
	once(f);
}

void session::open(const connection_info &ci) {
	conn_ = connections_manager::instance().open(ci);
}
void session::open(const std::string &cs) {
	CPPDB_LOG_DEBUG << "session: open";
	conn_ = connections_manager::instance().open(cs);
}
void session::close() {
	conn_.reset();
}

bool session::is_open() {
	return conn_ == nullptr;
}

statement session::prepare(const std::string &query) {
	CPPDB_LOG_DEBUG << "statement: create with query " << query;
	throw_guard g(conn_);
	std::shared_ptr<backend::statement> stat_ptr(conn_->prepare(query));
	statement stat(stat_ptr, conn_);
	return stat;
}

statement session::create_statement(const std::string &query) {
	throw_guard g(conn_);
	std::shared_ptr<backend::statement> stat_ptr(conn_->get_statement(query));
	statement stat(stat_ptr, conn_);
	return stat;
}

statement session::create_prepared_statement(const std::string &query) {
	throw_guard g(conn_);
	std::shared_ptr<backend::statement> stat_ptr(conn_->get_prepared_statement(query));
	statement stat(stat_ptr, conn_);
	return stat;
}

statement session::create_prepared_uncached_statement(const std::string &query) {
	throw_guard g(conn_);
	std::shared_ptr<backend::statement> stat_ptr(conn_->get_prepared_uncached_statement(query));
	statement stat(stat_ptr, conn_);
	return stat;
}

statement session::operator<<(const std::string &q) {
	return prepare(q);
}
statement session::operator<<(const char *s) {
	return prepare(s);
}
void session::begin() {
	throw_guard g(conn_);
	conn_->begin();
}
void session::commit() {
	throw_guard g(conn_);
	conn_->commit();
}
void session::rollback() {
	throw_guard g(conn_);
	conn_->rollback();
}
std::string session::escape(const char *b, const char *e) {
	return conn_->escape(b, e);
}
std::string session::escape(const char *s) {
	return conn_->escape(s);
}
std::string session::escape(const std::string &s) {
	return conn_->escape(s);
}
std::string session::driver() {
	return conn_->driver();
}
std::string session::engine() {
	return conn_->engine();
}

void session::once_called(bool v) {
	conn_->once_called(v);
}
bool session::once_called() {
	return conn_->once_called();
}

void session::once(const once_functor &f) {
	if (!once_called()) {
		f(*this);
		once_called(true);
	}
}

transaction::transaction(session &s) : s_(&s) {
	s_->begin();
}

void transaction::commit() {
	s_->commit();
	commited_ = true;
}
void transaction::rollback() {
	if (!commited_)
		s_->rollback();
	commited_ = true;
}
transaction::~transaction() {
	try {
		rollback();
	} catch (...) {}
}

void session::clear_cache() {
	conn_->clear_cache();
}

void session::clear_pool() {
	conn_->clear_cache();
	conn_->recyclable(false);
	conn_->get_pool()->clear();
}

bool session::recyclable() {
	return conn_->recyclable();
}
void session::recyclable(bool v) {
	conn_->recyclable(v);
}

std::shared_ptr<connection_specific_data> session::get_specific(const std::type_info &t) {
	return conn_->connection_specific_get(t);
}
std::shared_ptr<connection_specific_data> session::release_specific(const std::type_info &t) {
	return conn_->connection_specific_release(t);
}
void session::reset_specific(const std::type_info &t, std::shared_ptr<connection_specific_data> p) {
	conn_->connection_specific_reset(t, p);
}

const char *version_string() {
	return CPPDB_VERSION;
}
int version_number() {
	return CPPDB_MAJOR * 10000 + CPPDB_MINOR * 100 + CPPDB_PATCH;
}

} // namespace cppdb
