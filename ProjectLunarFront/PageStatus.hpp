#pragma once

#include "Page.hpp"

#include "JudgeRecord.hpp"
#include "Problem.hpp"

class PageStatus : public Page {
public:

	string getRequestPrefix() override { return "/status"; }

	HTTPResponseWrapper::Ptr getPage(const HTTPRequest& request) {
		string tr;
		const string& uri = request.GetURI();
		map<string, string> cookies, uriParams;
		decodeCookieAndUriParam(request, cookies, uriParams);
		int page = 1;
		if (auto i = uriParams.find("page"); i != uriParams.end())
			page = StringParser::toInt(i->second);
		// TODO Display all the results for now
		judgeRecordDatabase.lock();
		for (auto&[id, record] : judgeRecordDatabase.getRecords()) {
			Problem& prob = *problemDatabase.getProblem(record->probId);
			// format the submit time
			time_t t = (time_t)record->submitUnixTime;
			char tstr[48];
			strftime(tstr, sizeof(tstr), "%F %T", localtime(&t));
			tr += StringParser::toStringF(R"(<tr>
<td nowrap="nowrap" align="center"><a href="/statdetail/%d">%d</td>
<td nowrap="nowrap" align="center">%d</td>
<td nowrap="nowrap" align="center">%s</td>
<td nowrap="nowrap" align="center">%s</td>
<td nowrap="nowrap" align="center">%s</td>
<td nowrap="nowrap" align="center">%d/%d</td>
<td nowrap="nowrap" align="center">%dms</td>
<td nowrap="nowrap" align="center">%.2lfKB</td>
<td nowrap="nowrap" align="center">%s</td>
</tr>)", id, id, record->userId, prob.name.c_str(), prob.strid.c_str(), JudgeRecord::getStatusString(record->state).c_str(),
record->score, record->maxscore, record->maxTimeMs, (double)record->maxMemoryKb / 1024.0, tstr);
		}
		judgeRecordDatabase.unlock();
		// return the framed file
		return filetemplate("html/status_table.html", { { "%TR%", tr }, {"%TITLE%","Status Table"} });
	}
};

