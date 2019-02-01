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
		WrongAnswer,
		TimeLimitExceeded,
		MemoryLimitExceeded,
		RuntimeError,
		CompileError,
		FailedToRun,
		UnknownError,
	};

	struct DataPoint {
		string judgeMessage;
		State state;
		int maxscore, score;
		int memUsedKb, timeUsedMs;
	};

	int id;
	int probId, userId;
	Uuid codeStrDBId;

	State state;
	string compileMessage;
	vector<DataPoint> points;
	
	// sum of all points[].maxscore and points[].score, respectively
	int maxscore, score;
};

class JudgeRecordDatabase :public Lockable {
public:

	void loadFromFile(const wstring& filename);

	void saveToFile(const wstring& filename);

	int handInCode(int userId, int probId, const string& code, bool wantJudgeNow = true);

	// Not thread-safe; this->lock when reading
	auto& getWaitingQueue() { return waitingQueue; }

	// Not thread-safe; this->lock when reading
	auto& getRecords() { return records; }

private:
	int maxid;
	map<int, JudgeRecord::Ptr> records;
	deque<int> waitingQueue;
};

extern JudgeRecordDatabase judgeRecordDatabase;
