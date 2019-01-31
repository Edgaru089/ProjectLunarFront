
#include "JudgeWorker.hpp"
#include "JudgeRecord.hpp"

void JudgeWorker::judge(JudgeRecord::Ptr record) {

	// set up variables
	string srcFileNoExt = config.getTempDir() + generateCookie(16);
	string srcCodeFile = srcFileNoExt + ".cpp", srcExeFile = srcFileNoExt + ".cpp";


}
