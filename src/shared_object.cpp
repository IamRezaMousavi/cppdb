#define CPPDB_SOURCE

#include <cppdb/shared_object.hpp>

#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include <windows.h>
#define RTLD_LAZY 0

namespace cppdb {
namespace {
void *dlopen(const char *name, int /*unused*/) {
	return LoadLibrary(name);
}
void dlclose(void *h) {
	HMODULE m = (HMODULE)(h);
	FreeLibrary(m);
}
void *dlsym(void *h, const char *sym) {
	HMODULE m = (HMODULE)(h);
	return (void *)GetProcAddress(m, sym);
}
} // namespace
} // namespace cppdb

#else
#include <dlfcn.h>
#endif

namespace cppdb {
shared_object::shared_object(std::string name, void *h) : dlname_(name), handle_(h) {}
shared_object::~shared_object() {
	dlclose(handle_);
}
std::shared_ptr<shared_object> shared_object::open(const std::string &name) {
	std::shared_ptr<shared_object> dl;
	void *h = dlopen(name.c_str(), RTLD_LAZY);
	if (!h) {
		return dl;
	}
	try {
		dl.reset(new shared_object(name, h));
		h = nullptr;
		return dl;
	} catch (...) {
		if (h) {
			dlclose(h);
		}
		throw;
	}
}
void *shared_object::sym(const std::string &s) {
	return dlsym(handle_, s.c_str());
}
void *shared_object::safe_sym(const std::string &s) {
	void *p = sym(s);
	if (!p) {
		throw cppdb_error("cppdb::shared_object::failed to resolve symbol [" + s + "] in " + dlname_);
	}
	return p;
}
} // namespace cppdb
