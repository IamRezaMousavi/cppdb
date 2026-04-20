#ifndef CPPDB_SHARED_OBJECT_HPP
#define CPPDB_SHARED_OBJECT_HPP

#include <cppdb/defs.h>
#include <cppdb/errors.hpp>

#include <memory>
#include <string>

namespace cppdb {

///
/// \brief This class allows to load and unload shared objects in simple and exception safe way.
///
class CPPDB_API shared_object {
	shared_object(const std::string &name, void *h);

public:
	shared_object(const shared_object &) = delete;
	void operator=(const shared_object &) = delete;

	~shared_object();
	///
	/// Load shared object, returns empty pointer if the object does not exits or not loadable
	///
	static std::shared_ptr<shared_object> open(const std::string &name);
	///
	/// Resolve symbol \a name and return pointer on it, throws cppdb_error if the symbol can't be resolved
	///
	void *safe_sym(const std::string &name);

	///
	/// Resolve symbol \a name and return pointer on it, returns NULL if the symbol can't be resolved
	///
	void *sym(const std::string &name);

	///
	/// Resolve symbol \a name and assign it to \a v, returns false if the symbol can't be resolved
	///
	template <typename T>
	bool resolve(const std::string &s, T *&v) {
		void *p = sym(s);
		if (!p) {
			return false;
		}
		v = (T *)(p);
		return true;
	}
	///
	/// Resolve symbol \a name and assign it to v, throws cppdb_error if the symbol can't be resolved
	///
	template <typename T>
	void safe_resolve(const std::string &s, T *&v) {
		v = (T *)(sym(s));
	}

private:
	std::string dlname_;
	void *handle_ = nullptr;
};

} // namespace cppdb

#endif /* CPPDB_SHARED_OBJECT_HPP */
