#ifndef CPPDB_DRIVER_MANAGER_HPP
#define CPPDB_DRIVER_MANAGER_HPP

#include <cppdb/defs.h>
#include <cppdb/ref_ptr.hpp>

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace cppdb {

namespace backend {
class connection;
class driver;
} // namespace backend
class connection_info;

///
/// \brief this class is used to handle all drivers, loading them, unloading them etc.
///
/// All its member functions are thread safe
///
class CPPDB_API driver_manager {
public:
	///
	/// Get the singleton instance of the class
	///
	static driver_manager &instance();
	///
	/// Install new driver \a drv named \a name to the manager.
	///
	void install_driver(const std::string &name, ref_ptr<backend::driver> drv);
	///
	/// Unload all drivers that have no more open connections.
	///
	void collect_unused();

	///
	/// Add a path were the driver should search for loadable modules
	///
	void add_search_path(const std::string &);
	///
	/// Clear previously added a paths
	///
	void clear_search_paths();
	///
	/// Search the library under default directory (i.e. empty path prefix) or not, default is true
	///
	void use_default_search_path(bool v);
	///
	/// Create a new connection object using parsed connection string \a ci
	///
	backend::connection *connect(const connection_info &ci);
	///
	/// Create a new connection object using connection string \a connectoin_string
	///
	backend::connection *connect(const std::string &connectoin_string);

private:
	driver_manager(const driver_manager &);
	void operator=(const driver_manager &);
// Borland erros on hidden destructors in classes without only static methods.
#ifndef __BORLANDC__
	~driver_manager();
#endif
	driver_manager();

	ref_ptr<backend::driver> load_driver(const connection_info &ci);

	typedef std::map<std::string, ref_ptr<backend::driver> > drivers_type;
	std::vector<std::string> search_paths_;
	bool no_default_directory_;
	drivers_type drivers_;
	std::mutex lock_;
};

} // namespace cppdb

#endif /* CPPDB_DRIVER_MANAGER_HPP */
