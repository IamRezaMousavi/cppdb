#ifndef CPPDB_MUTEX_H
#define CPPDB_MUTEX_H

#include <cppdb/defs.h>

namespace cppdb {

	///
	/// \brief mutex class, used internally
	///
	class CPPDB_API mutex {
		mutex(mutex const &);
		void operator=(mutex const  &);
	public:
		class guard;
		/// Create mutex
		mutex();
		/// Destroy mutex
		~mutex();
		/// Lock mutex
		void lock();
		/// Unlock mutex
		void unlock();
	private:
		void *mutex_impl_;
	};

	///
	/// \brief scoped guard for mutex
	///
	class mutex::guard {
		guard(guard const &);
		void operator=(guard const &);
	public:
		/// Create scoped lock
		guard(mutex &m) : m_(&m)
		{
			m_->lock();
		}
		/// unlock the mutex
		~guard()
		{
			m_->unlock();
		}
	private:
		mutex *m_;
	};
}
#endif
