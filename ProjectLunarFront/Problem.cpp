
#include "Problem.hpp"

ProblemDatabase problemDatabase;


bool ProblemDatabase::loadFromFile(const wstring& filename) {
	lock_guard<Lockable>(*this);
	mlog << "[ProblemDatabase] Loading from file " << filename << dlog;

	loaded = false;
	problems.clear();

	OptionFile opt;
	int cnt;

	if (!opt.loadFromFile(filename))
		return false;
	cnt = StringParser::toInt(opt.getContent("count"));

	for (int i = 1; i <= cnt; i++) {
		mlog << "[ProblemDatabase] Loading problem " << i << dlog;
		string prefix = StringParser::toStringF("prob%d_", i);
		auto prob = make_shared<Problem>();

		prob->id = i;
		prob->name = opt.getContent(prefix + "name");
		prob->strid = opt.getContent(prefix + "strid");
		prob->targetInFileName = opt.getContent(prefix + "in_data");
		if (prob->targetInFileName.empty())
			prob->targetInFileName = prob->strid + ".in";
		prob->targetOutFileName = opt.getContent(prefix + "out_data");
		if (prob->targetOutFileName.empty())
			prob->targetOutFileName = prob->strid + ".out";
		prob->inDataPathFormat = opt.getContent(prefix + "in_data_fmt");
		prob->outDataPathFormat = opt.getContent(prefix + "out_data_fmt");
		prob->dataIdFirst = StringParser::toInt(opt.getContent(prefix + "data_id_first"));
		prob->dataIdLast = StringParser::toInt(opt.getContent(prefix + "data_id_last"));
		prob->dataCnt = prob->dataIdLast - prob->dataIdFirst + 1;
		prob->judgeMethod = Problem::ByLineIgnoreTrailingSpaces;
		if (string str = opt.getContent(prefix + "compare_method"); !str.empty()) {
			if (str == "byline")
				prob->judgeMethod = Problem::ByLineIgnoreTrailingSpaces;
			else if (str == "bybyte")
				prob->judgeMethod = Problem::ByByte;
			else if (str == "realnumber")
				prob->judgeMethod = Problem::RealNumber;
		}

		int defScore = 10, defTime = 1000, defMem = 262144; // 256MB
		if (string str = opt.getContent(prefix + "default_perdata_score"); !str.empty())
			defScore = StringParser::toInt(str);
		if (string str = opt.getContent(prefix + "default_time_ms"); !str.empty())
			defTime = StringParser::toInt(str);
		if (string str = opt.getContent(prefix + "default_memory_kb"); !str.empty())
			defMem = StringParser::toInt(str);

		prob->points.resize(prob->dataCnt);
		prob->totalScore = 0;
		for (int i = prob->dataIdFirst; i <= prob->dataIdLast; i++) {
			auto& p = prob->points[i - prob->dataIdFirst];
			string prep = prefix + "data" + to_string(i) + '_';
			p.totalScore = defScore;
			p.timeLimitMs = defTime;
			p.memoryLimitKb = defMem;
			if (string str = opt.getContent(prep + "score"); !str.empty())
				p.totalScore = StringParser::toInt(str);
			if (string str = opt.getContent(prep + "time_ms"); !str.empty())
				p.timeLimitMs = StringParser::toInt(str);
			if (string str = opt.getContent(prep + "memory_kb"); !str.empty())
				p.memoryLimitKb = StringParser::toInt(str);
			prob->totalScore += p.totalScore;
		}

		// TODO get the raw markdown string
		problems.insert(make_pair(i, prob));
	}

	loaded = true;
	mlog << "[ProblemDatabase] File loaded. " << cnt << " problems." << dlog;
	return true;
}

