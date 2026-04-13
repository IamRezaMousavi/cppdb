#include <cppdb/backend.hpp>

namespace dummy {

int results = 0;
int statements = 0;
int connections = 0;
int drivers = 0;

class result : public cppdb::backend::result {
public:
	virtual next_row has_next() {
		return next_row_unknown;
	}
	virtual bool next() {
		if (called_)
			return false;
		called_ = true;
		return true;
	}
	virtual bool fetch(int, short &) {
		return false;
	}
	virtual bool fetch(int, unsigned short &) {
		return false;
	}
	virtual bool fetch(int, int &) {
		return false;
	}
	virtual bool fetch(int, unsigned &) {
		return false;
	}
	virtual bool fetch(int, long &) {
		return false;
	}
	virtual bool fetch(int, unsigned long &) {
		return false;
	}
	virtual bool fetch(int, long long &) {
		return false;
	}
	virtual bool fetch(int, unsigned long long &) {
		return false;
	}
	virtual bool fetch(int, float &) {
		return false;
	}
	virtual bool fetch(int, double &) {
		return false;
	}
	virtual bool fetch(int, long double &) {
		return false;
	}
	virtual bool fetch(int, std::string &) {
		return false;
	}
	virtual bool fetch(int, std::ostream &) {
		return false;
	}
	virtual bool fetch(int, std::tm &) {
		return false;
	}
	virtual bool is_null(int) {
		return true;
	}
	virtual int cols() {
		return 10;
	}
	virtual int name_to_column(const std::string &) {
		return -1;
	}
	virtual std::string column_to_name(int) {
		throw cppdb::not_supported_by_backend("unsupported");
	}

	// End of API
	result() : called_(false) {
		results++;
	}
	~result() {
		results--;
	}

private:
	bool called_;
};

class statement : public cppdb::backend::statement {
public:
	virtual void reset() {}
	const std::string &sql_query() {
		return q_;
	}
	virtual void bind(int, const std::string &) {}
	virtual void bind(int, const char *) {}
	virtual void bind(int, const char *, const char *) {}
	virtual void bind(int, const std::tm &) {}
	virtual void bind(int, std::istream &) {}
	virtual void bind(int, int) {}
	virtual void bind(int, unsigned) {}
	virtual void bind(int, long) {}
	virtual void bind(int, unsigned long) {}
	virtual void bind(int, long long) {}
	virtual void bind(int, unsigned long long) {}
	virtual void bind(int, double) {}
	virtual void bind(int, long double) {}
	virtual void bind_null(int) {}
	virtual long long sequence_last(const std::string & /*sequence*/) {
		throw cppdb::not_supported_by_backend("unsupported");
	}
	virtual unsigned long long affected() {
		return 0;
	}
	virtual std::shared_ptr<cppdb::backend::result> query() {
		return std::make_shared<result>();
	}
	virtual void exec() {}
	statement(const std::string &q) : q_(q) {
		statements++;
	}
	~statement() {
		statements--;
	}

private:
	std::string q_;
};

extern "C" {
typedef cppdb::backend::connection *cppdb_backend_connect_function(const cppdb::connection_info &ci);
}

class connection : public cppdb::backend::connection {
public:
	connection(const cppdb::connection_info &info) : cppdb::backend::connection(info) {
		connections++;
	}
	~connection() {
		connections--;
	}
	virtual void begin() {}
	virtual void commit() {}
	virtual void rollback() {}
	virtual statement *prepare_statement(const std::string &q) {
		return new statement(q);
	}
	virtual statement *create_statement(const std::string &q) {
		return new statement(q);
	}
	virtual std::string escape(const std::string &) {
		throw cppdb::not_supported_by_backend("not supported");
	}
	virtual std::string escape(const char *) {
		throw cppdb::not_supported_by_backend("not supported");
	}
	virtual std::string escape(const char *, const char *) {
		throw cppdb::not_supported_by_backend("not supported");
	}
	virtual std::string driver() {
		return "dummy";
	}
	virtual std::string engine() {
		return "dummy";
	}
};

class loadable_driver : public cppdb::backend::loadable_driver {
public:
	loadable_driver() {
		drivers++;
	}
	connection *open(const cppdb::connection_info &cs) {
		return new connection(cs);
	}
	~loadable_driver() {
		drivers--;
	}
};

} // namespace dummy

extern "C" {
cppdb::backend::connection *dummy_connect_function(const cppdb::connection_info &ci) {
	return new dummy::connection(ci);
}
}
