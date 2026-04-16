#ifndef CPPDB_POOL_HPP
#define CPPDB_POOL_HPP

#include <cppdb/defs.h>
#include <cppdb/utils.hpp>

#include <list>
#include <memory>
#include <mutex>

namespace cppdb {

class connection_info;
namespace backend {
class connection;
}

///
/// \brief Connections pool, allows to handle multiple connections for specific connection string.
///
/// Note \ref connections_manager provide more generic interface and hides pools inside it. So you
/// generally should use this class only when you prefer to avoid using some global singleton object.
///
/// Unlike \ref connections_manager, it uses pool by default unless its size defined as 0.
///
/// All this class member functions are thread safe to use from several threads for the same object
///
class CPPDB_API pool : public std::enable_shared_from_this<pool> {
public:
	pool(const pool &) = delete;
	void operator=(const pool &) = delete;

	pool(const connection_info &ci);
	/// Create new pool for \a connection_string
	static std::shared_ptr<pool> create(const std::string &connection_string);
	/// Create new pool for a parsed connection string \a ci
	static std::shared_ptr<pool> create(const connection_info &ci);

	///
	/// Shortcut of std::shared_ptr<cppdb::pool> as cppdb::pool::pointer.
	///
	/// The pointer that is used to handle pool object
	///
	typedef std::shared_ptr<pool> pointer;

	~pool();

	///
	/// Get a open a connection, it may be fetched either from pool or new one may be created
	///
	std::shared_ptr<backend::connection> open();
	///
	/// Collect connections that were not used for a long time (close them)
	///
	void gc();

	///
	/// Remove all connections from the pool
	///
	void clear();

	/// \cond INTERNAL
	void put(backend::connection *c_in);
	/// \endcond
private:
	std::shared_ptr<backend::connection> get();

	struct data;
	std::unique_ptr<data> d;

	struct entry {
		std::shared_ptr<backend::connection> conn;
		std::time_t last_used = 0L;
	};

	typedef std::list<entry> pool_type;
	// non-mutable members

	size_t limit_ = 0;
	int life_time_ = 0;
	connection_info ci_;

	// mutex protected begin
	std::mutex lock_;
	size_t size_ = 0;
	pool_type pool_;
	// mutex protected end
};

} // namespace cppdb

#endif /* CPPDB_POOL_HPP */
