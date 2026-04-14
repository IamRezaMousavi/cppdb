#ifndef CPPDB_CONNECTION_SPECIFIC_HPP
#define CPPDB_CONNECTION_SPECIFIC_HPP

#include <cppdb/defs.h>

#include <memory>

namespace cppdb {

///
/// \brief Special abstract object that holds a connection specific data
///
/// The user is expected to derive its own object from this class
/// and save them withing the connection
///
class CPPDB_API connection_specific_data {
public:
	connection_specific_data(const connection_specific_data &) = delete;
	void operator=(const connection_specific_data &) = delete;

	connection_specific_data();
	virtual ~connection_specific_data();

private:
	struct data;
	std::unique_ptr<data> d;
};

} // namespace cppdb

#endif /* CPPDB_CONNECTION_SPECIFIC_HPP */
