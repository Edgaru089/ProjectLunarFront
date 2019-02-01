
#include "JudgeRecord.hpp"

#include "StringDatabase.hpp"
#include "Problem.hpp"

JudgeRecordDatabase judgeRecordDatabase;


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
	lock();

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

	rec->points.resize(prob.dataCnt, JudgeRecord::DataPoint{ "", JudgeRecord::Waiting, 0, 0, 0, 0 });
	for (int i = prob.dataIdFirst; i <= prob.dataIdLast; i++)
		rec->points[i - prob.dataIdFirst].maxscore = prob.points[i - prob.dataIdFirst].totalScore;

	// insert the JudgeRecord
	records.insert(make_pair(id, rec));

	// if wantJudgeNow: queue the record
	if (wantJudgeNow)
		waitingQueue.push_back(id);

	unlock();
	return id;
}

