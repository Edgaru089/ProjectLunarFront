#pragma once

#include <memory>
#include <mutex>
#include "Main.hpp"

class Lockable {
public:

	Lockable() { __mutex = std::make_shared<std::recursive_mutex>(); }
	virtual ~Lockable() {}

	void lock() { ASSERT(__mutex); __mutex->lock(); }
	void unlock() { ASSERT(__mutex); __mutex->unlock(); }
	bool try_lock() { ASSERT(__mutex); return __mutex->try_lock(); }
	void* native_handle() { ASSERT(__mutex); return __mutex->native_handle(); }

private:
	std::shared_ptr<std::recursive_mutex> __mutex;
};

