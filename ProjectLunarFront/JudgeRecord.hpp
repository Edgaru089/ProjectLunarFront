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
		Count
	};

	static const string& getStatusString(State state);

	struct DataPoint {
		string judgeMessage;
		State state;
		int maxscore, score;
		int memUsedKb, timeUsedMs;
	};

	int id;
	int probId, userId;
	Uuid codeStrDBId;

	// unix time
	Uint64 submitUnixTime;

	State state;
	string compileMessage;
	vector<DataPoint> points;

	// sum of all points[].maxscore and points[].score, respectively
	int maxscore, score;

	int maxTimeMs, maxMemoryKb;
};

class JudgeRecordDatabase :public Lockable {
public:

	void loadFromFile(const wstring& filename);

	void saveToFile(const wstring& filename);

	int handInCode(int userId, int probId, const string& code, bool wantJudgeNow = true);

	void requestRejudge(int recordId, bool pushFront = false);

	// Not thread-safe; this->lock when reading
	auto& getWaitingQueue() { return waitingQueue; }

	// Not thread-safe; this->lock when reading
	auto& getRecords() { return records; }

private:
	int maxid;
	map<int, JudgeRecord::Ptr, greater<int>> records;
	deque<int> waitingQueue;
};

extern JudgeRecordDatabase judgeRecordDatabase;
