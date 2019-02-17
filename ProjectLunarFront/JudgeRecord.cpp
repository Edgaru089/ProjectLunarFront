
#include "JudgeRecord.hpp"

#include "StringDatabase.hpp"
#include "Problem.hpp"

JudgeRecordDatabase judgeRecordDatabase;

namespace {
	const string stateStrings[] = {
		"Waiting"s,
		"Compiling"s,
		"Running"s,
		"Accepted"s,
		"Wrong Answer"s,
		"Time Limit Exceeded"s,
		"Memory Limit Exceeded"s,
		"Runtime Error"s,
		"Compile Error"s,
		"Program Cannot Start"s,
		"Unknown Internal Error"s
	};
}


const string& JudgeRecord::getStatusString(State state) {
	return stateStrings[(int)state];
}


void JudgeRecordDatabase::loadFromFile(const wstring& filename) {
	lock();
	mlog << "[JudgeRecordDatabase] Loading judge records from " << filename << dlog;

	throw NotImplementedException("JudgeRecordDatabase::loadFromFile");

	mlog << "[JudgeRecordDatabase] File loaded." << dlog;
	unlock();
}


void JudgeRecordDatabase::saveToFile(const wstring& filename) {
	lock();
	mlog << "[JudgeRecordDatabase] Saving judge records to " << filename << dlog;

	throw NotImplementedException("JudgeRecordDatabase::saveToFile");

	mlog << "[JudgeRecordDatabase] File saved." << dlog;
	unlock();
}


int JudgeRecordDatabase::handInCode(int userId, int probId, const string& code, bool wantJudgeNow) {
	lock_guard<Lockable>(*this);

	// register the string at StringDB
	Uuid strid = stringDatabase.insert(code);

	// get the problem
	const Problem& prob = *problemDatabase.getProblem(probId);

	// create the JudgeRecord
	JudgeRecord::Ptr rec = make_shared<JudgeRecord>();
	int id = rec->id = ++maxid;
	rec->userId = userId;
	rec->probId = probId;
	rec->codeStrDBId = strid;
	rec->state = JudgeRecord::Waiting;
	rec->submitUnixTime = time(nullptr); // returns the UTC Unix time, seconds from 1970.1.1 00:00

	rec->points.resize(prob.dataCnt, JudgeRecord::DataPoint{ "", JudgeRecord::Waiting, 0, 0, 0, 0 });
	for (int i = prob.dataIdFirst; i <= prob.dataIdLast; i++)
		rec->points[i - prob.dataIdFirst].maxscore = prob.points[i - prob.dataIdFirst].totalScore;

	// insert the JudgeRecord
	records.insert(make_pair(id, rec));

	// if wantJudgeNow: queue the record
	if (wantJudgeNow)
		waitingQueue.push_back(id);

	return id;
}


void JudgeRecordDatabase::requestRejudge(int recordId, bool pushFront) {
	lock_guard<Lockable>(*this);

	auto p = records.find(recordId)->second;
	for (auto& i : p->points)
		i = JudgeRecord::DataPoint{ "", JudgeRecord::Waiting, 0, 0, 0, 0 };

	if (pushFront)
		waitingQueue.push_front(recordId);
	else
		waitingQueue.push_back(recordId);
}

