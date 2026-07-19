#include <cppdb/backend.hpp>
#include <cppdb/logging.hpp>
#include <cppdb/pool.hpp>
#include <cppdb/utils.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <memory>

namespace cppdb {
namespace backend {

// statement
void statement::cache(statements_cache *c) {
	cache_ = c;
}

void statement::dispose(statement *p) {
	if (!p)
		return;
	statements_cache *cache = p->cache_;
	p->cache_ = 0;
	if (cache)
		cache->put(p);
	else
		delete p;
}

// statements cache//////////////

struct statements_cache::data {
	struct entry;
	typedef std::map<std::string, entry> statements_type;
	typedef std::list<statements_type::iterator> lru_type;
	struct entry {
		std::shared_ptr<statement> stat;
		lru_type::iterator lru_ptr;
	};

	statements_type statements;
	lru_type lru;
	size_t size = 0;
	size_t max_size = 0;

	void insert(const std::shared_ptr<statement> &st) {
		statements_type::iterator p;
		if ((p = statements.find(st->sql_query())) != statements.end()) {
			p->second.stat = st;
			lru.erase(p->second.lru_ptr);
			lru.push_front(p);
			p->second.lru_ptr = lru.begin();
		} else {
			if (size > 0 && size >= max_size) {
				statements.erase(lru.back());
				lru.pop_back();
				size--;
			}
			std::pair<statements_type::iterator, bool> ins =
				statements.insert(std::make_pair(st->sql_query(), entry()));
			p = ins.first;
			p->second.stat = st;
			lru.push_front(p);
			p->second.lru_ptr = lru.begin();
			size++;
		}
	}

	std::shared_ptr<statement> fetch(const std::string &query) {
		std::shared_ptr<statement> st;
		statements_type::iterator p = statements.find(query);
		if (p == statements.end())
			return st;
		st = p->second.stat;
		lru.erase(p->second.lru_ptr);
		statements.erase(p);
		size--;
		return st;
	}

	void clear() {
		lru.clear();
		statements.clear();
		size = 0;
	}
}; // data

void statements_cache::set_size(size_t n) {
	if (n != 0 && !active()) {
		d.reset(new data());
		d->max_size = n;
	}
}
void statements_cache::put(statement *p_in) {
	if (!active()) {
		delete p_in;
		p_in = nullptr;
	}
	std::shared_ptr<statement> p = make_stmt<statement>(p_in);
	p->reset();
	d->insert(p);
}
std::shared_ptr<statement> statements_cache::fetch(const std::string &q) {
	if (!active())
		return nullptr;
	return d->fetch(q);
}
void statements_cache::clear() {
	d->clear();
}

bool statements_cache::active() {
	return d.get() != nullptr;
}

//////////////
// connection
//////////////

struct connection::data {
	typedef std::list<std::shared_ptr<connection_specific_data>> conn_specific_type;
	conn_specific_type conn_specific;
};
std::shared_ptr<statement> connection::prepare(const std::string &q) {
	if (default_is_prepared_)
		return get_prepared_statement(q);
	else
		return get_statement(q);
}

std::shared_ptr<statement> connection::get_statement(const std::string &q) {
	std::shared_ptr<statement> st = create_statement(q);
	return st;
}

std::shared_ptr<statement> connection::get_prepared_statement(const std::string &q) {
	std::shared_ptr<statement> st;
	if (!cache_.active()) {
		st = prepare_statement(q);
		return st;
	}
	st = cache_.fetch(q);
	if (!st)
		st = prepare_statement(q);
	st->cache(&cache_);
	return st;
}

std::shared_ptr<statement> connection::get_prepared_uncached_statement(const std::string &q) {
	std::shared_ptr<statement> st = prepare_statement(q);
	return st;
}

connection::connection(const connection_info &info) : d(new connection::data), pool_(0) {
	int cache_size = info.get("@stmt_cache_size", 64);
	if (cache_size > 0) {
		cache_.set_size(cache_size);
	}
	std::string def_is_prep = info.get("@use_prepared", "on");
	if (def_is_prep == "on")
		default_is_prepared_ = true;
	else if (def_is_prep == "off")
		default_is_prepared_ = false;
	else
		throw cppdb_error("cppdb::backend::connection: @use_prepared should be either 'on' or 'off'");
	CPPDB_LOG_INFO << "conn: create new one";
}
connection::~connection() {
	CPPDB_LOG_INFO << "conn: destory";
}

bool connection::once_called() const {
	return once_called_;
}
void connection::once_called(bool v) {
	once_called_ = v;
}
std::shared_ptr<connection_specific_data> connection::connection_specific_get(const std::type_info &type) const {
	auto it = std::find_if(
		d->conn_specific.begin(), d->conn_specific.end(),
		[&](const std::shared_ptr<connection_specific_data> &p) { return p != nullptr && typeid(*p) == type; });
	if (it != d->conn_specific.end()) {
		return *it;
	}
	return nullptr;
}
std::shared_ptr<connection_specific_data> connection::connection_specific_release(const std::type_info &type) {
	auto it = std::find_if(
		d->conn_specific.begin(), d->conn_specific.end(),
		[&](const std::shared_ptr<connection_specific_data> &p) { return p != nullptr && typeid(*p) == type; });
	if (it != d->conn_specific.end()) {
		auto ptr = *it;
		d->conn_specific.erase(it);
		return ptr;
	}
	return nullptr;
}
void connection::connection_specific_reset(const std::type_info &type,
										   const std::shared_ptr<connection_specific_data> ptr) {
	if (ptr && typeid(*ptr) != type) {
		throw cppdb_error(std::string("cppdb::connection_specific::Inconsistent pointer type") + typeid(*ptr).name()
						  + " and std::type_info reference:" + type.name());
	}
	auto it = std::find_if(
		d->conn_specific.begin(), d->conn_specific.end(),
		[&](const std::shared_ptr<connection_specific_data> &p) { return p != nullptr && typeid(*p) == type; });
	if (it != d->conn_specific.end()) {
		it->reset();
		if (ptr)
			*it = ptr;
		else
			d->conn_specific.erase(it);
		return;
	}
	if (ptr) {
		d->conn_specific.push_back(ptr);
	}
}

std::shared_ptr<pool> connection::get_pool() {
	return pool_;
}
void connection::set_pool(const std::shared_ptr<pool> &p) {
	pool_ = p;
}
void connection::clear_cache() {
	cache_.clear();
}

void connection::recyclable(bool opt) {
	recyclable_ = opt;
}

bool connection::recyclable() {
	return recyclable_;
}

void connection::dispose(connection *c) {
	if (!c)
		return;
	std::shared_ptr<pool> p = c->pool_;
	c->pool_ = nullptr;
	if (p && c->recyclable())
		p->put(c);
	else {
		c->clear_cache();
		delete c;
	}
}

std::shared_ptr<connection> driver::connect(const connection_info &cs) {
	return open(cs);
}

} // namespace backend

struct connection_specific_data::data {};

connection_specific_data::connection_specific_data() {}
connection_specific_data::~connection_specific_data() {}

} // namespace cppdb
