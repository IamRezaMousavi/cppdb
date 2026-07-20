#ifndef CPPDB_CONN_MANAGER_HPP
#define CPPDB_CONN_MANAGER_HPP

#include <cppdb/backend.hpp>
#include <cppdb/pool.hpp>
#include <cppdb/utils.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace cppdb {

///
/// \brief This class is the major gateway to new connections
///
/// It handles connection pools and forwards request to the drivers.
///
/// This class member functions are thread safe
///
class connections_manager {
	connections_manager() = default;

public:
	connections_manager(const connections_manager &) = delete;
	void operator=(const connections_manager &) = delete;
	///
	/// Get a singleton instance of the class
	///
	static connections_manager &instance();
	///
	/// Create a new connection using connection string \a cs
	///
	std::shared_ptr<backend::connection> open(const std::string &cs);
	///
	/// Create a new connection using parsed connection string \a ci
	///
	std::shared_ptr<backend::connection> open(const connection_info &ci);
	///
	/// Collect all connections that were not used for long time and close them.
	///
	void gc();

private:
	std::mutex lock_;
	using connections_type = std::map<std::string, std::shared_ptr<pool>>;
	connections_type connections_;
};

} // namespace cppdb

#endif /* CPPDB_CONN_MANAGER_HPP */
