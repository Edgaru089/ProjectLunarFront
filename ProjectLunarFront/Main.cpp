#include "Main.hpp"
#include "Instance.hpp"

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
	OPEN_IFSTREAM_WSTR(logout, buffer, ofstream::binary);
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

	Instance instance;
	instance.start(Instance::Config{});

	while (running)
		this_thread::sleep_for(chrono::milliseconds(50));

	instance.stop();

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

