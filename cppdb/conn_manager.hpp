#ifndef CPPDB_CONN_MANAGER_HPP
#define CPPDB_CONN_MANAGER_HPP

#include <cppdb/defs.h>
#include <cppdb/ref_ptr.hpp>

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace cppdb {

class pool;
class connection_info;
namespace backend {
class connection;
}

///
/// \brief This class is the major gateway to new connections
///
/// It handles connection pools and forwards request to the drivers.
///
/// This class member functions are thread safe
///
class CPPDB_API connections_manager {
	connections_manager();
// Borland erros on hidden destructors in classes without only static methods.
#ifndef __BORLANDC__
	~connections_manager();
#endif
	connections_manager(connections_manager const &);
	void operator=(const connections_manager &);

public:
	///
	/// Get a singleton instance of the class
	///
	static connections_manager &instance();
	///
	/// Create a new connection using connection string \a cs
	///
	ref_ptr<backend::connection> open(const std::string &cs);
	///
	/// Create a new connection using parsed connection string \a ci
	///
	ref_ptr<backend::connection> open(const connection_info &ci);
	///
	/// Collect all connections that were not used for long time and close them.
	///
	void gc();

private:
	struct data;
	std::unique_ptr<data> d;

	std::mutex lock_;
	typedef std::map<std::string, std::shared_ptr<pool> > connections_type;
	connections_type connections_;
};

} // namespace cppdb

#endif /* CPPDB_CONN_MANAGER_HPP */
