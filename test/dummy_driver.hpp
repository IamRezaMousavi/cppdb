#include <cppdb/backend.hpp>
#include <cppdb/driver_manager.hpp>

#include <memory>

namespace dummy {

int results = 0;
int statements = 0;
int connections = 0;
int drivers = 0;

class result : public cppdb::backend::result {
public:
	next_row has_next() override {
		return next_row_unknown;
	}
	bool next() override {
		if (called_)
			return false;
		called_ = true;
		return true;
	}
	bool fetch(int, short &) override {
		return false;
	}
	bool fetch(int, unsigned short &) override {
		return false;
	}
	bool fetch(int, int &) override {
		return false;
	}
	bool fetch(int, unsigned &) override {
		return false;
	}
	bool fetch(int, long &) override {
		return false;
	}
	bool fetch(int, unsigned long &) override {
		return false;
	}
	bool fetch(int, long long &) override {
		return false;
	}
	bool fetch(int, unsigned long long &) override {
		return false;
	}
	bool fetch(int, float &) override {
		return false;
	}
	bool fetch(int, double &) override {
		return false;
	}
	bool fetch(int, long double &) override {
		return false;
	}
	bool fetch(int, std::string &) override {
		return false;
	}
	bool fetch(int, std::ostream &) override {
		return false;
	}
	bool fetch(int, std::tm &) override {
		return false;
	}
	bool is_null(int) override {
		return true;
	}
	int cols() override {
		return 10;
	}
	int name_to_column(const std::string &) override {
		return -1;
	}
	std::string column_to_name(int) override {
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
	void reset() override {}
	const std::string &sql_query() override {
		return q_;
	}
	void bind(int, const std::string &) override {}
	void bind(int, const char *) override {}
	void bind(int, const char *, const char *) override {}
	void bind(int, const std::tm &) override {}
	void bind(int, std::istream &) override {}
	void bind(int, int) override {}
	void bind(int, unsigned) override {}
	void bind(int, long) override {}
	void bind(int, unsigned long) override {}
	void bind(int, long long) override {}
	void bind(int, unsigned long long) override {}
	void bind(int, double) override {}
	void bind(int, long double) override {}
	void bind_null(int) override {}
	long long sequence_last(const std::string & /*sequence*/) override {
		throw cppdb::not_supported_by_backend("unsupported");
	}
	unsigned long long affected() override {
		return 0;
	}
	std::shared_ptr<cppdb::backend::result> query() override {
		return std::make_shared<result>();
	}
	void exec() override {}
	statement(const std::string &q) : q_(q) {
		statements++;
	}
	~statement() {
		statements--;
	}

private:
	std::string q_;
};

class connection : public cppdb::backend::connection {
public:
	connection(const cppdb::connection_info &info) : cppdb::backend::connection(info) {
		connections++;
	}
	~connection() {
		connections--;
	}
	void begin() override {}
	void commit() override {}
	void rollback() override {}
	std::shared_ptr<cppdb::backend::statement> prepare_statement(const std::string &q) override {
		return cppdb::backend::make_stmt<statement>(q);
	}
	std::shared_ptr<cppdb::backend::statement> create_statement(const std::string &q) override {
		return cppdb::backend::make_stmt<statement>(q);
	}
	std::string escape(const std::string &) override {
		throw cppdb::not_supported_by_backend("not supported");
	}
	std::string escape(const char *) override {
		throw cppdb::not_supported_by_backend("not supported");
	}
	std::string escape(const char *, const char *) override {
		throw cppdb::not_supported_by_backend("not supported");
	}
	std::string driver() override {
		return "dummy";
	}
	std::string engine() override {
		return "dummy";
	}
};

class driver final : public cppdb::backend::driver {
public:
	driver() {
		drivers++;
	}
	std::shared_ptr<cppdb::backend::connection> open(const cppdb::connection_info &ci) override {
		return cppdb::backend::make_conn<connection>(ci);
	}
	~driver() {
		drivers--;
	}
};

} // namespace dummy

namespace {

struct register_mysql {
	register_mysql() {
		cppdb::driver_manager::instance().install_driver("dummy", std::make_shared<dummy::driver>());
	}
} reg;

} // namespace
