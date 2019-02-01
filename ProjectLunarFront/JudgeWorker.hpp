#pragma once

#include "Main.hpp"

class JudgeRecord;
class Problem;

// Here, 'target' means contestant output; 'std' means standard data stored elsewhere
class JudgeWorker {
public:
	static void judge(shared_ptr<JudgeRecord> record);

private:
	static fs::path compile(shared_ptr<JudgeRecord> record, const wstring& targetPathNoExt);
	static void judgeOneSet(const fs::path& exe, const fs::path& workingDir, const fs::path& targetIn, const fs::path& targetOut, int id, shared_ptr<JudgeRecord> record, const Problem& prob);

	static void compareByByte(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record);
	static void compareByLine(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record);
	static void compareRealNumber(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record);

	static void judgeThreadWorker(int id, atomic_bool& running);

public:

	void launch(unsigned int workerCount = thread::hardware_concurrency());
	void stop();

	bool isRunning() { return running; }

private:

	vector<shared_ptr<thread>> workers;
	atomic_bool running;

};

extern JudgeWorker judgeWorker;

