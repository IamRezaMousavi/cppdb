#ifndef CPPDB_CONNECTION_SPECIFIC_H
#define CPPDB_CONNECTION_SPECIFIC_H

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
		connection_specific_data(connection_specific_data const &);
		void operator=(connection_specific_data const &);
	public:
		connection_specific_data();
		virtual ~connection_specific_data();

	private:
		struct data;
		std::unique_ptr<data> d;
	};


} // cppdb

#endif
