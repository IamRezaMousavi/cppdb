#ifndef CPPDB_UTIL_HPP
#define CPPDB_UTIL_HPP

#include <cppdb/defs.h>

#include <ctime>
#include <map>
#include <string>

namespace cppdb {

///
/// \brief parse a string as time value.
///
/// Used by backend implementations;
///
std::tm parse_time(const char *value);
///
/// \brief format a string as time value.
///
/// Used by backend implementations;
///
std::string format_time(const std::tm &v);
///
/// \brief parse a string as time value.
///
/// Used by backend implementations;
///
std::tm parse_time(const std::string &v);

///
/// \brief Parse a connection string \a cs into driver name \a driver_name and list of properties \a props
///
/// The connection string format is following:
///
/// \verbatim  driver:[key=value;]*  \endverbatim
///
/// Where value can be either a sequence of characters (white space is trimmed) or it may be a general
/// sequence encloded in a single quitation marks were double quote is used for insering a single quote value.
///
/// Key values starting with \@ are reserved to be used as special cppdb  keys
/// For example:
///
/// \verbatim   mysql:username= root;password = 'asdf''5764dg';database=test;@use_prepared=off' \endverbatim
///
/// Where driver is "mysql", username is "root", password is "asdf'5764dg", database is "test" and
/// special value "@use_prepared" is off - internal cppdb option.
void parse_connection_string(const std::string &cs, std::string &driver_name,
									   std::map<std::string, std::string> &props);

///
/// \brief Class that represents parsed connection string
///
class connection_info {
public:
	///
	/// The original connection string
	///
	std::string connection_string;
	///
	/// The driver name
	///
	std::string driver;
	///
	/// Type that represent key, values set
	///
	typedef std::map<std::string, std::string> properties_type;
	///
	/// The std::map of key value properties.
	///
	properties_type properties;

	///
	/// Cheks if property \a prop, has been given in connection string.
	///
	bool has(const std::string &prop) const;
	///
	/// Get property \a prop, returning \a default_value if not defined.
	///
	std::string get(const std::string &prop, const std::string &default_value = std::string()) const;
	///
	/// Get numeric value for property \a prop, returning \a default_value if not defined.
	/// If the value is not a number, throws cppdb_error.
	///
	int get(const std::string &prop, int default_value) const;

	///
	/// Default constructor - empty info
	///
	connection_info() = default;
	///
	/// Create connection_info from the connection string parsing it.
	///
	explicit connection_info(const std::string &cs) : connection_string(cs) {
		parse_connection_string(cs, driver, properties);
	}
};

} // namespace cppdb

#endif /* CPPDB_UTIL_HPP */
