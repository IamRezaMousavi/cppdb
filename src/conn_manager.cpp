#define CPPDB_SOURCE

#include <cppdb/backend.hpp>
#include <cppdb/conn_manager.hpp>
#include <cppdb/driver_manager.hpp>
#include <cppdb/pool.hpp>

namespace cppdb {
connections_manager::connections_manager() {}

connections_manager &connections_manager::instance() {
	static connections_manager mgr;
	return mgr;
}

namespace {
struct init {
	init() {
		connections_manager::instance();
	}
} initializer;
} // namespace

std::shared_ptr<backend::connection> connections_manager::open(const std::string &cs) {
	std::shared_ptr<pool> p;
	/// seems we may be using pool
	if (cs.find("@pool_size") != std::string::npos) {
		std::lock_guard<std::mutex> lock(lock_);
		connections_type::iterator pool_ptr = connections_.find(cs);
		if (pool_ptr != connections_.end())
			p = pool_ptr->second;
	}

	if (p) {
		return p->open();
	} else {
		connection_info ci(cs);
		return open(ci);
	}
}
std::shared_ptr<backend::connection> connections_manager::open(const connection_info &ci) {
	if (ci.get("@pool_size", 0) == 0) {
		return driver_manager::instance().connect(ci);
	}
	std::shared_ptr<pool> p;
	{
		std::lock_guard<std::mutex> lock(lock_);
		std::shared_ptr<pool> &ref_p = connections_[ci.connection_string];
		if (!ref_p) {
			ref_p = pool::create(ci);
		}
		p = ref_p;
	}
	return p->open();
}
void connections_manager::gc() {
	std::vector<std::shared_ptr<pool> > pools_;
	pools_.reserve(100);
	{
		std::lock_guard<std::mutex> lock(lock_);
		for (connections_type::iterator p = connections_.begin(); p != connections_.end(); ++p) {
			pools_.push_back(p->second);
		}
	}
	for (unsigned i = 0; i < pools_.size(); i++) {
		pools_[i]->gc();
	}
	pools_.clear();
	{
		std::lock_guard<std::mutex> lock(lock_);
		for (connections_type::iterator p = connections_.begin(); p != connections_.end();) {
			if (p->second.unique()) {
				pools_.push_back(p->second);
				connections_type::iterator tmp = p;
				++p;
				connections_.erase(tmp);
			} else
				++p;
		}
	}
	pools_.clear();
}

} // namespace cppdb
