#pragma once

#include "Main.hpp"

class JudgeRecord;

class JudgeWorker {
public:
	static void judge(shared_ptr<JudgeRecord> record);
};
