#pragma once

#include "Main.hpp"

#include "JudgeRecord.hpp"
#include "Problem.hpp"
#include "Page.hpp"
#include "StringDatabase.hpp"

class PageStatusDetail :public Page {
public:

	string getRequestPrefix() override { return "/statdetail/"; }

	HTTPResponseWrapper::Ptr getPage(const HTTPRequest& request) {
		string ft, tr;
		const string& uri = request.GetURI();
		map<string, string> cookies, uriParams;
		decodeCookieAndUriParam(request, cookies, uriParams);
		int id = StringParser::toInt(uri.substr(uri.find_last_of('/') + 1));
		judgeRecordDatabase.lock();

		auto i = judgeRecordDatabase.getRecords().find(id);
		if (i == judgeRecordDatabase.getRecords().end()) {
			judgeRecordDatabase.unlock();
			return filetemplate("html/general_div.html", { { "%CONT%", "Error: Record Not Found" } });
		}

		auto rec = i->second;
		Problem& prob = *problemDatabase.getProblem(rec->probId);

		ft = StringParser::toStringF("Status #%d: Problem %s(%s), User %d; Status: %s (%d/%d)",
			i, prob.name.c_str(), prob.strid.c_str(), rec->userId, JudgeRecord::getStatusString(rec->state), rec->score, rec->maxscore);

		if (rec->state == JudgeRecord::CompileError) {
			judgeRecordDatabase.unlock();
			return filetemplate("html/status_detail_noscore.html", {
				{ "%TITLE%", StringParser::toStringF("Status Detail #%d", i) },
				{ "%NAV%", getUserNavString(cookies) },
				{ "%MSG%", escapeTextHTML(rec->compileMessage) },
				{ "%CODE%", escapeTextHTML(stringDatabase.get(rec->codeStrDBId).getString()) }
			});
		}

		for (int i = prob.dataIdFirst; i <= prob.dataIdLast; i++) {
			JudgeRecord::DataPoint p = rec->points[i - prob.dataIdFirst];
			tr += StringParser::toStringF(R"(<tr>
<td nowrap="nowrap" align="center">#%d</td>
<td nowrap="nowrap" align="center">%s</td>
<td nowrap="nowrap" align="center">%.2fMB</td>
<td nowrap="nowrap" align="center">%dms</td>
<td nowrap="nowrap" align="center">%d/%d</td>
</tr>)", i, JudgeRecord::getStatusString(p.state).c_str(), (float)p.memUsedKb / 1024.0f, p.timeUsedMs, p.score, p.maxscore);
		}

		judgeRecordDatabase.unlock();
		// return the framed file
		return filetemplate("html/status_detail.html", {
			{ "%TITLE%", StringParser::toStringF("Status Detail #%d", i) },
			{ "%NAV%", getUserNavString(cookies) },
			{ "%TR%", tr },
			{ "%CODE%", escapeTextHTML(stringDatabase.get(rec->codeStrDBId).getString()) }
		});
	}
};
