#ifndef CPPDB_LOGGING_HPP
#define CPPDB_LOGGING_HPP

#include <memory>
#include <sstream>
#include <string>
#include <cstdint>

namespace cppdb {

enum class LogLevel : std::uint8_t {
	Debug = 0,
	Info,
	Warning,
	Error,
	Critical
};

class ILogHandler {
public:
	virtual ~ILogHandler() = default;

	virtual void log(LogLevel level, const std::string &message) = 0;
};

class logger {
public:
	logger(LogLevel level) : level_(level) {}

	~logger() {
		auto handler = get_handler();
		if (handler && level_ >= get_current_log_level()) {
			handler->log(level_, stringstream_.str());
		}
	}

	template <typename T>
	logger &operator<<(const T &value) {
		if (level_ >= get_current_log_level()) {
			stringstream_ << value;
		}
		return *this;
	}

	static void setLogLevel(LogLevel level) {
		get_log_level_ref() = level;
	}

	static void set_handler(std::shared_ptr<ILogHandler> handler) {
		get_handler_ref() = std::move(handler);
	}

	static std::shared_ptr<ILogHandler> get_handler() {
		return get_handler_ref();
	}

	static LogLevel get_current_log_level() {
		return get_log_level_ref();
	}

private:
	static LogLevel &get_log_level_ref() {
		static LogLevel current_level = LogLevel::Info;
		return current_level;
	}

	static std::shared_ptr<ILogHandler> &get_handler_ref() {
		static std::shared_ptr<ILogHandler> handler = nullptr;
		return handler;
	}

private:
	std::ostringstream stringstream_;
	LogLevel level_;
};

} // namespace cppdb

#define CPPDB_LOG_CRITICAL                                                   \
	if (cppdb::logger::get_current_log_level() <= cppdb::LogLevel::Critical) \
	cppdb::logger(cppdb::LogLevel::Critical)
#define CPPDB_LOG_ERROR                                                   \
	if (cppdb::logger::get_current_log_level() <= cppdb::LogLevel::Error) \
	cppdb::logger(cppdb::LogLevel::Error)
#define CPPDB_LOG_WARNING                                                   \
	if (cppdb::logger::get_current_log_level() <= cppdb::LogLevel::Warning) \
	cppdb::logger(cppdb::LogLevel::Warning)
#define CPPDB_LOG_INFO                                                   \
	if (cppdb::logger::get_current_log_level() <= cppdb::LogLevel::Info) \
	cppdb::logger(cppdb::LogLevel::Info)
#define CPPDB_LOG_DEBUG                                                   \
	if (cppdb::logger::get_current_log_level() <= cppdb::LogLevel::Debug) \
	cppdb::logger(cppdb::LogLevel::Debug)

#endif /* CPPDB_LOGGING_HPP */
