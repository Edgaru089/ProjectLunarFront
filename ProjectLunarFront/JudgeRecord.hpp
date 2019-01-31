#pragma once

#include "Main.hpp"
#include "Lockable.hpp"

class JudgeRecord :public Lockable, public enable_shared_from_this<JudgeRecord> {
public:

	typedef shared_ptr<JudgeRecord> Ptr;

	enum State {
		Waiting,
		Compiling,
		Running,
		Accepted,
		FailedToRun,
		WrongAnswer,
		TimeLimitExceeded,
		MemoryLimitExceeded,
		UnknownError,
	};

	void judge();

	int recordId, probId, userId;
	Uuid codeStrDBId;
	State state;
};

class JudgeRecordDatabase :public Lockable {
public:

	void saveToFile(const string& filename) {
		lock();
		mlog << "[JudgeRecordDatabase] Saving judge records to " << filename << dlog;

		mlog << "[JudgeRecordDatabase] File saved." << dlog;
		unlock();
	}

private:
	map<Uuid, JudgeRecord::Ptr> records;
};
