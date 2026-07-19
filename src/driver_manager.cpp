#include <cppdb/backend.hpp>
#include <cppdb/driver_manager.hpp>
#include <cppdb/utils.hpp>

namespace cppdb {

std::shared_ptr<backend::connection> driver_manager::connect(const std::string &str) {
	connection_info conn(str);
	return connect(conn);
}

std::shared_ptr<backend::connection> driver_manager::connect(const connection_info &conn) {
	std::shared_ptr<backend::driver> drv_ptr;
	{ // get driver
		std::lock_guard<std::mutex> lock(lock_);
		auto p = drivers_.find(conn.driver);
		if (p != drivers_.end()) {
			drv_ptr = p->second;
		} else {
			throw cppdb_error("cppdb::driver_manager::connect: Can't find driver");
		}
	}
	return drv_ptr->connect(conn);
}

void driver_manager::install_driver(const std::string &name, const std::shared_ptr<backend::driver> &drv) {
	if (!drv) {
		throw cppdb_error("cppdb::driver_manager::install_driver: Can't install empty driver");
	}
	std::lock_guard<std::mutex> lock(lock_);
	drivers_[name] = drv;
}

driver_manager &driver_manager::instance() {
	static driver_manager instance;
	return instance;
}

} // namespace cppdb
