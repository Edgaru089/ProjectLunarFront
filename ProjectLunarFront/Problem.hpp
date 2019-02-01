#pragma once

#include "Main.hpp"

class Problem :public enable_shared_from_this<Problem> {
public:

	typedef shared_ptr<Problem> Ptr;

	enum JudgeMethod {
		ByByte,
		ByLineIgnoreTrailingSpaces,
		RealNumber,  // < 1e-6
	};

	struct DataPoint {
		int timeLimitMs;
		int memoryLimitKb;
		int totalScore;
	};

	int id;
	string name, strid;
	string rawMarkdownStr;

	JudgeMethod judgeMethod;

	// add.in, add.out, etc.
	string targetInFileName, targetOutFileName;

	// "data/add/add%02d.in" , etc.
	string inDataPathFormat, outDataPathFormat;
	//  [1, 10] (inclusive) 10 (calculated)
	int dataIdFirst, dataIdLast, dataCnt;
	// stored <as sequence>; vec[i-dataIdFirst]
	vector<DataPoint> points;
	// sum of all points[].totalScore
	int totalScore;
};

class ProblemDatabase :public Lockable {
public:

	bool loadFromFile(const wstring& filename);

	Problem::Ptr getProblem(int id) {
		auto i = problems.find(id);
		if (i == problems.end())
			return nullptr;
		else
			return i->second;
	}

	bool isLoaded() { return loaded; }

private:
	map<int, Problem::Ptr> problems;
	bool loaded = false;
};

extern ProblemDatabase problemDatabase;

