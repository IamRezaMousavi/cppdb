#include <cppdb/backend.hpp>
#include <cppdb/driver_manager.hpp>
#include <cppdb/logging.hpp>
#include <cppdb/pool.hpp>
#include <cppdb/utils.hpp>

#include <ctime>

namespace cppdb {

std::shared_ptr<pool> pool::create(const connection_info &ci) {
	auto p = std::make_shared<pool>(ci);
	return p;
}
std::shared_ptr<pool> pool::create(const std::string &cs) {
	connection_info ci(cs);
	auto p = std::make_shared<pool>(ci);
	return p;
}

pool::pool(const connection_info &ci) : ci_(ci) {
	limit_ = ci_.get("@pool_size", 16);
	life_time_ = ci_.get("@pool_max_idle", 600);
	CPPDB_LOG_INFO << "pool: create with limit " << limit_;
}

pool::~pool() {
	CPPDB_LOG_INFO << "pool: desetory";
}

std::shared_ptr<backend::connection> pool::open() {
	if (limit_ == 0)
		return driver_manager::instance().connect(ci_);

	std::shared_ptr<backend::connection> p = get();

	if (!p) {
		CPPDB_LOG_DEBUG << "pool: pool connection is empty";
		p = driver_manager::instance().connect(ci_);
	} else {
		CPPDB_LOG_INFO << "pool: use connection";
	}
	p->set_pool(shared_from_this());
	return p;
}

// this is thread safe member function
std::shared_ptr<backend::connection> pool::get() {
	if (limit_ == 0)
		return 0;
	std::shared_ptr<backend::connection> c;
	pool_type garbage;
	std::time_t now = time(0);
	{
		std::lock_guard<std::mutex> lock(lock_);
		// Nothing there should throw so it is safe
		pool_type::iterator p = pool_.begin(), tmp;
		while (p != pool_.end()) {
			if (p->last_used + life_time_ < now) {
				tmp = p;
				p++;
				garbage.splice(garbage.begin(), pool_, tmp);
				size_--;
			} else {
				// all is sorted by time
				break;
			}
		}
		if (!pool_.empty()) {
			c = pool_.back().conn;
			pool_.pop_back();
			size_--;
		}
	}
	return c;
}

// this is thread safe member function
void pool::put(backend::connection *c_in) {
	CPPDB_LOG_INFO << "pool: put connection to pool";
	std::shared_ptr<backend::connection> c = backend::make_conn<backend::connection>(c_in);
	if (limit_ == 0)
		return;
	pool_type garbage;
	std::time_t now = time(0);
	{
		std::lock_guard<std::mutex> lock(lock_);
		// under lock do all very fast
		if (c.get()) {
			pool_.push_back(entry());
			pool_.back().last_used = now;
			pool_.back().conn = c;
			size_++;
		}

		// Nothing there should throw so it is safe

		pool_type::iterator p = pool_.begin(), tmp;
		while (p != pool_.end()) {
			if (p->last_used + life_time_ < now) {
				tmp = p;
				p++;
				garbage.splice(garbage.begin(), pool_, tmp);
				size_--;
			} else {
				// all is sorted by time
				break;
			}
		}
		// can be at most 1 entry bigger then limit
		if (size_ > limit_) {
			garbage.splice(garbage.begin(), pool_, pool_.begin());
			size_--;
		}
	}
}

void pool::gc() {
	put(nullptr);
}

void pool::clear() {
	pool_type garbage;
	{
		std::lock_guard<std::mutex> lock(lock_);
		garbage.swap(pool_);
		size_ = 0;
	} // destroy outside mutex scope
}
} // namespace cppdb
