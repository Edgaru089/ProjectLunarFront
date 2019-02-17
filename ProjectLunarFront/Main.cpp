#include "Main.hpp"
#include "Instance.hpp"

#include "JudgeWorker.hpp"
#include "JudgeRecord.hpp"
#include "StringDatabase.hpp"
#include "Problem.hpp"
#include "Page.hpp"
#include "PageStatus.hpp"
#include "PageStatusDetail.hpp"

Log dlog;
ofstream logout;

atomic_bool running, stopped;

#ifdef _WIN32
//Platform-Depedent: Windows
BOOL systemExitEventHandler(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_C_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-C Console Exit" << dlog;
	else if (dwCtrlType == CTRL_BREAK_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-Break Console Exit" << dlog;
	else if (dwCtrlType == CTRL_CLOSE_EVENT)
		mlog << Log::Error << "[Main/EVENT] Control-Close Console Exit" << dlog;
	else if (dwCtrlType == CTRL_LOGOFF_EVENT)
		mlog << Log::Error << "[Main/EVENT] System-Logoff Exit" << dlog;
	else if (dwCtrlType == CTRL_SHUTDOWN_EVENT)
		mlog << Log::Error << "[Main/EVENT] System-Shutdown Exit" << dlog;
	else
		return false;
	running = false;
	while (!stopped)
		this_thread::sleep_for(chrono::milliseconds(50));
	return true;
}
#else
// Assuming a POSIX system
void sigintHandler(int signal) {
	if (signal == SIGINT)
		mlog << Log::Error << "[Main/EVENT] POSIX SIGINT Exit" << dlog;
	else if (signal == SIGTERM)
		mlog << Log::Error << "[Main/EVENT] POSIX SIGTERM Exit" << dlog;
	running = false;
}
#endif


int main(int argc, char* argv[]) try {
	// Open a binary output stream for logs
	/*time_t curtime = time(NULL);
	wchar_t buffer[64] = {};
	char signature[] = u8"\ufeff"; // BOM in UTF-8; "signature"
	wcsftime(buffer, 63, L"logs/%Y-%m-%d-%H.%M.%S.log", localtime(&curtime));
	OPEN_FSTREAM_WSTR(logout, buffer, ofstream::binary);
	logout.write(signature, sizeof(signature) - 1);*/

#if (defined WIN32) || (defined _WIN32)
	locale::global(locale("", LC_CTYPE));
	wcout.imbue(locale("", LC_CTYPE));
#else
	setlocale(LC_CTYPE, "zh_CN.utf8");
#endif
	dlog.addOutputStream(wcout);
	/*dlog.addOutputHandler([&](const wstring& str) {
		string u8str = wstringToUtf8(str) + "\r\n";
		logout.write(u8str.data(), u8str.size());
		logout.flush();
	});*/

	running = true;
	stopped = false;
#ifdef _WIN32
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)systemExitEventHandler, true);
#else
	signal(SIGINT, sigintHandler);
	signal(SIGTERM, sigintHandler);
#endif

	mlog << completeServerNameEx << dlog;


	Instance instance;

	setFrameFile(StringParser::replaceSubString(readFileText(L"html/frame.html"), { { "%FOOTER%", completeServerNameEx } }));

	// Register routes here
	instance.registerRouteRule("/static/", config.getHostname(), ROUTER(req) {
		return file(req.GetURI().substr(1));
	});

	instance.registerRouteRule("/sanae", config.getHostname(), ROUTER(req) {
		return htmltext("Kochiya Sanae");
	});
	pageManager.insertPage(make_shared<PageStatus>());
	pageManager.insertPage(make_shared<PageStatusDetail>());
	pageManager.registerRoutes(instance);

	instance.start(Instance::Config{});

//	while (running)
//		this_thread::sleep_for(chrono::milliseconds(50));

	string word;
	while (cin >> word && running) {
		if (word == "stop")
			break;
		else if (word == "load") {
			stringDatabase.initializeWithFolder("objects");
			judgeWorker.launch(1);

			sleep(milliseconds(500));

			wstring str;
			cout << "problem db: ";
			wcin >> str;
			problemDatabase.loadFromFile(str);

			cout << "complete." << endl;
		} else if (word == "judge") {
			int id;
			wstring str;
			cout << "problem id: ";
			cin >> id;
			cout << "code file: ";
			wcin >> str;

			cout << "handing in..." << endl;
			judgeRecordDatabase.handInCode(0, id, readFileText(str), true);

			cout << "complete." << endl;
		}
	}

	running = false;

	//instance.stop();
	judgeWorker.stop();

	stopped = true;

	return 0;
} catch (exception& e) {
	mlog << Log::Error << "The main closure failed with an uncaught exception: " << e.what() << dlog;
	mlog << Log::Error << "Exception typename: " << typeid(e).name()
#if (defined WIN32) || (defined _WIN32)
		<< " (" << typeid(e).raw_name() << ")"
#endif
		<< dlog;
	mlog << Log::Error << "The program will now fail." << dlog;
	return 1;
} catch (...) {
	mlog << Log::Error << "The main closure failed with an unknown object thrown." << dlog;
	mlog << Log::Error << "The program will now fail." << dlog;
	return 1;
}

