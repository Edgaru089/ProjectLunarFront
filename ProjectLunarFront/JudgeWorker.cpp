
#include "JudgeWorker.hpp"

#include "JudgeRecord.hpp"
#include "Problem.hpp"
#include "StringDatabase.hpp"

#ifdef OS_WINDOWS
#include <Psapi.h>
#endif

JudgeWorker judgeWorker;


fs::path JudgeWorker::compile(shared_ptr<JudgeRecord> record, const wstring& targetPathNoExt) {
	mlog << "                     Compiling..." << dlog;

	record->state = JudgeRecord::Compiling;

	// set up variables
	const Problem& prob = *problemDatabase.getProblem(record->probId);

	ASSERT(config.getTempDir().back() == '/' && "Config::TempDir Must End With Slash");
#ifdef OS_WINDOWS
	fs::path srcCodeFile = targetPathNoExt + L".cpp", srcExeFile = targetPathNoExt + L".exe";
	fs::path compileOut = targetPathNoExt + L".compile.out";
#else
	fs::path srcCodeFile = wstringToUtf8(targetPathNoExt + L".cpp"), srcExeFile = wstringToUtf8(targetPathNoExt + L".exe");
	fs::path compileOut = wstringToUtf8(targetPathNoExt + L".compile.out");
#endif

	// write the code file
	writeFileText(srcCodeFile.generic_wstring(), stringDatabase.get(record->codeStrDBId).getString());

	// craft the compile command
	int retval = 0;
#ifdef OS_WINDOWS
	wstring command = StringParser::replaceSubStringW(
		utf8ToWstring(config.getCppCompiler()),
		  { { L"%CODE", L'"' + srcCodeFile.native() + L'"' },
		  { L"%EXE", L'"' + srcExeFile.native() + L'"' } }) + L" 1> \"" + compileOut.native() + L"\" 2>&1";
	retval = _wsystem(command.c_str());
#else
	string command = StringParser::replaceSubString(
		config.getCppCompiler(),
			{ { "%CODE", srcCodeFile.generic_u8string() },
			{ "%EXE", srcExeFile.generic_u8string() } }) + " &> \"" + compileOut.native() + '"';
	retval = system(command.c_str());
#endif

	if (retval != 0) {
		mlog << "                     Compile Error!" << dlog;
		record->state = JudgeRecord::CompileError;
	}
	record->compileMessage = readFileText(compileOut.generic_wstring());
	mlog << "                     Compiler message: " << record->compileMessage << dlog;

	return srcExeFile;
}


void JudgeWorker::judge(JudgeRecord::Ptr record) {
	mlog << "[JudgeWorker::judge] Judging record " << record->id << " starting..." << dlog;

	// make sure the tempdir exists
	fs::path tempdir = config.getTempDir() + generateCookie(24);
	if (!fs::exists(tempdir) || !fs::is_directory(tempdir)) {
		fs::remove(tempdir);
		fs::create_directory(tempdir);
	}

	// set up variables
	const Problem& prob = *problemDatabase.getProblem(record->probId);

	// compile
	fs::path exeFile = compile(record, tempdir.generic_wstring() + '/' + prob.strid);
	if (record->state == JudgeRecord::CompileError) {
		for (int i = prob.dataIdFirst; i <= prob.dataIdLast; i++)
			record->points[i - prob.dataIdFirst].state = JudgeRecord::CompileError;
		return;
	}
	ASSERT(fs::exists(exeFile));

	fs::path targetIn = tempdir / prob.targetInFileName, targetOut = tempdir / prob.targetOutFileName;

	// judge!
	mlog << "                     Start judging..." << dlog;
	record->state = JudgeRecord::Running;
	for (int i = prob.dataIdFirst; i <= prob.dataIdLast; i++) {
		mlog << "                     Judging set " << i << dlog;
		judgeOneSet(exeFile, tempdir, targetIn, targetOut, i, record, prob);
		mlog << "                         Returned with result " << (int)record->points[i - prob.dataIdFirst].state << ", message: " << record->points[i - prob.dataIdFirst].judgeMessage << dlog;
	}

	// tell the judge result
	record->maxscore = record->score = 0;
	for (int i = 0; i < prob.dataCnt; i++) {
		auto& point = record->points[i];
		record->maxscore += point.maxscore;
		record->score += point.score;
	}
	if (record->state == JudgeRecord::Running) {
		record->state = JudgeRecord::Accepted;
		for (int i = 0; i < prob.dataCnt; i++) {
			auto& point = record->points[i];
			if (point.state != JudgeRecord::Accepted) {
				record->state = point.state;
				break;
			}
		}
	}

	// remove judged files
	fs::remove_all(tempdir);

	mlog << "[JudgeWorker::judge] Final result for " << record->id << ": " << record->score << '/' << record->maxscore << ", " << (int)record->state << dlog;
}


void JudgeWorker::compareByByte(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record) {
	auto& point = record->points[seqid];

	string target = readFileBinary(targetOut.generic_wstring());
	string stds = readFileBinary(stdOut.generic_wstring());
#define JUDGE(result, message) do { point.score = 0; point.state = JudgeRecord::result; point.judgeMessage = message; return; } while (false)

	if (target.size() > stds.size())
		JUDGE(WrongAnswer, "Longer than standard output");
	else if (target.size() < stds.size())
		JUDGE(WrongAnswer, "Shorter than standard output");
	else {
		for (int i = 0; i < target.size(); i++)
			if (target[i] != stds[i])
				JUDGE(WrongAnswer, StringParser::toStringF("Differ on byte #%d: std: %d, you: %d", i, (int)stds[i], (int)target[i]));
	}

	point.score = point.maxscore;
	point.state = JudgeRecord::Accepted;

#undef JUDGE
}


void JudgeWorker::compareRealNumber(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record) {
	throw NotImplementedException("JudgeWorker::compareRealNumber");
}


void JudgeWorker::compareByLine(const fs::path& stdOut, const fs::path& targetOut, int seqid, shared_ptr<JudgeRecord> record) {
	auto& point = record->points[seqid];

	// copied from lemon
	FILE *contestantOutputFile = fopen(targetOut.generic_u8string().c_str(), "r");
	if (contestantOutputFile == NULL) {
		point.score = 0;
		point.state = JudgeRecord::WrongAnswer;
		point.judgeMessage = "Cannot open contestant\'s output file";
		return;
	}
	FILE *standardOutputFile = fopen(stdOut.generic_u8string().c_str(), "r");
	if (standardOutputFile == NULL) {
		point.score = 0;
		point.state = JudgeRecord::UnknownError;
		point.judgeMessage = "Cannot open standard output file";
		fclose(contestantOutputFile);
		return;
	}

	char str1[20], str2[20], ch;
	bool chk1 = false, chk2 = false;
	bool chkEof1 = false, chkEof2 = false;
	int len1, len2;
	while (true) {
		len1 = 0;
		while (len1 < 10) {
			ch = fgetc(contestantOutputFile);
			if (ch == EOF) break;
			if (!chk1 && ch == '\n') break;
			if (chk1 && ch == '\n') {
				chk1 = false;
				continue;
			}
			if (ch == '\r') {
				chk1 = true;
				break;
			}
			if (chk1) chk1 = false;
			str1[len1++] = ch;
		}
		str1[len1++] = '\0';
		if (ch == EOF) chkEof1 = true; else chkEof1 = false;

		len2 = 0;
		while (len2 < 10) {
			ch = fgetc(standardOutputFile);
			if (ch == EOF) break;
			if (!chk2 && ch == '\n') break;
			if (chk2 && ch == '\n') {
				chk2 = false;
				continue;
			}
			if (ch == '\r') {
				chk2 = true;
				break;
			}
			if (chk2) chk2 = false;
			str2[len2++] = ch;
		}
		str2[len2++] = '\0';
		if (ch == EOF) chkEof2 = true; else chkEof2 = false;

		if (chkEof1 && !chkEof2) {
			point.score = 0;
			point.state = JudgeRecord::WrongAnswer;
			point.judgeMessage = "Shorter than standard output";
			fclose(contestantOutputFile);
			fclose(standardOutputFile);
			return;
		}

		if (!chkEof1 && chkEof2) {
			point.score = 0;
			point.state = JudgeRecord::WrongAnswer;
			point.judgeMessage = "Longer than standard output";
			fclose(contestantOutputFile);
			fclose(standardOutputFile);
			return;
		}

		if (len1 != len2 || strcmp(str1, str2) != 0) {
			point.score = 0;
			point.state = JudgeRecord::WrongAnswer;
			point.judgeMessage = StringParser::toStringF("Read %s but expect %s", str1, str2);
			fclose(contestantOutputFile);
			fclose(standardOutputFile);
			return;
		}
		if (chkEof1 && chkEof2) break;
	}

	point.score = point.maxscore;
	point.state = JudgeRecord::Accepted;
	point.judgeMessage = "Output ok";
	fclose(contestantOutputFile);
	fclose(standardOutputFile);
}


void JudgeWorker::judgeOneSet(const fs::path& exe, const fs::path& workingDir, const fs::path& targetIn, const fs::path& targetOut, int id, shared_ptr<JudgeRecord> record, const Problem& prob) {
	ASSERT(fs::exists(exe) && fs::is_regular_file(exe));

	// prepare variables
	wstring tempDir = exe.parent_path().native();
	fs::path stdIn = StringParser::toStringF(prob.inDataPathFormat.c_str(), id), stdOut = StringParser::toStringF(prob.outDataPathFormat.c_str(), id);
	int seqid = id - prob.dataIdFirst;

	// copy the input file
	fs::remove(targetIn);
	fs::remove(targetOut);
	fs::copy(stdIn, targetIn);

	auto& point = record->points[seqid];
	point.maxscore = prob.points[seqid].totalScore;

	auto& probp = prob.points[seqid];

	// run!
	point.state = JudgeRecord::Running;
#ifdef OS_WINDOWS
	// Copy code from Lemon::judgingthread.cpp
	SetErrorMode(SEM_NOGPFAULTERRORBOX);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES sa;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	ZeroMemory(&pi, sizeof(pi));
	ZeroMemory(&sa, sizeof(sa));
	sa.bInheritHandle = TRUE;

	si.hStdError = CreateFile((const WCHAR*)((tempDir + L"_tmperr").c_str()), GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &sa,
							  CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!CreateProcess(NULL, (LPWSTR)exe.native().c_str(), NULL, &sa,
		TRUE, HIGH_PRIORITY_CLASS | CREATE_NO_WINDOW, 0,
		(const WCHAR*)(workingDir.native().c_str()), &si, &pi)) {
		CloseHandle(si.hStdError);
		point.score = 0;
		record->points[seqid].state = JudgeRecord::FailedToRun;
		return;
	}

	PROCESS_MEMORY_COUNTERS_EX info;
	ZeroMemory(&info, sizeof(info));
	info.cb = sizeof(info);
	if (probp.memoryLimitKb != -1) {
		GetProcessMemoryInfo(pi.hProcess, (PROCESS_MEMORY_COUNTERS*)&info, sizeof(info));
		if (max(info.PrivateUsage, info.PeakWorkingSetSize) > probp.memoryLimitKb * 1024) {
			TerminateProcess(pi.hProcess, 0);
			CloseHandle(si.hStdError);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			point.score = 0;
			point.state = JudgeRecord::MemoryLimitExceeded;
			point.memUsedKb = point.timeUsedMs = -1;
			return;
		}
	}

	bool flag = false;
	Clock timer;
	timer.restart();

	while (timer.getElapsedTime().asMilliseconds() <= probp.timeLimitMs) {
		if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0) {
			flag = true;
			break;
		}
		if (probp.memoryLimitKb != -1) {
			GetProcessMemoryInfo(pi.hProcess, (PROCESS_MEMORY_COUNTERS*)&info, sizeof(info));
			if (max(info.PrivateUsage, info.PeakWorkingSetSize) > probp.memoryLimitKb * 1024) {
				TerminateProcess(pi.hProcess, 0);
				CloseHandle(si.hStdError);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				point.score = 0;
				point.state = JudgeRecord::MemoryLimitExceeded;
				point.memUsedKb = point.timeUsedMs = -1;
				return;
			}
		}
		sleep(milliseconds(10));
	}

	if (!flag) {
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(si.hStdError);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		point.score = 0;
		point.state = JudgeRecord::TimeLimitExceeded;
		point.timeUsedMs = -1;
		return;
	}

	unsigned long exitCode;
	GetExitCodeProcess(pi.hProcess, &exitCode);
	if (exitCode != 0) {
		CloseHandle(si.hStdError);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		point.score = 0;
		point.state = JudgeRecord::RuntimeError;
		fs::path file(tempDir + L"_tmperr");
		if (fs::exists(file))
			point.judgeMessage = readFileText(file.generic_wstring());
		point.memUsedKb = point.timeUsedMs = -1;
		return;
	}

	FILETIME creationTime, exitTime, kernelTime, userTime;
	GetProcessTimes(pi.hProcess, &creationTime, &exitTime, &kernelTime, &userTime);

	SYSTEMTIME realTime;
	FileTimeToSystemTime(&userTime, &realTime);

	point.timeUsedMs = realTime.wMilliseconds
		+ realTime.wSecond * 1000
		+ realTime.wMinute * 60 * 1000
		+ realTime.wHour * 60 * 60 * 1000;

	GetProcessMemoryInfo(pi.hProcess, (PROCESS_MEMORY_COUNTERS*)&info, sizeof(info));
	point.memUsedKb = info.PeakWorkingSetSize / 1024;

	CloseHandle(si.hStdError);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

#else
	throw NotImplementedException("JudgeWorker::judgeOneSet for non-Windows platforms");
#endif

	// read the output
	switch (prob.judgeMethod) {
		case Problem::ByByte:
			compareByByte(stdOut, targetOut, seqid, record);
			break;
		case Problem::ByLineIgnoreTrailingSpaces:
			compareByLine(stdOut, targetOut, seqid, record);
			break;
		case Problem::RealNumber:
			compareRealNumber(stdOut, targetOut, seqid, record);
			break;
	}

}


void JudgeWorker::launch(unsigned int workerCount) {
	running = true;
	workers.resize(workerCount);
	for (int i = 0; i < workerCount; i++)
		workers[i] = make_shared<thread>(JudgeWorker::judgeThreadWorker, i, ref(running));
}

void JudgeWorker::stop() {
	running = false;
	for (auto p : workers)
		if (p->joinable())
			p->join();
}

void JudgeWorker::judgeThreadWorker(int id, atomic_bool& running) {
	mlog << "[JudgeThreadWorker " << id << "] Started" << dlog;
	while (running) {

		auto& q = judgeRecordDatabase.getWaitingQueue();

		JudgeRecord::Ptr rec = nullptr;

		judgeRecordDatabase.lock();
		if (!q.empty()) {
			rec = judgeRecordDatabase.getRecords().find(q.front())->second;
			q.pop_front();
		}
		judgeRecordDatabase.unlock();

		if (rec) {
			mlog << "[JudgeThreadWorker " << id << "] Got waiting judge " << rec->id << " from queue, judging now" << dlog;
			judge(rec);
		}

		this_thread::sleep_for(chrono::milliseconds(10));
	}
}

