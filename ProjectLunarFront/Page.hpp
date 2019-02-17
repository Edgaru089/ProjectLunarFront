#pragma once

#include "Main.hpp"
#include "Instance.hpp"


class Page : public enable_shared_from_this<Page> {
public:

	// Return the prefix string that will be used to match the requests handled
	virtual string getRequestPrefix() = 0;

	// Return the request method that the page would listen to
	// Defaults to GET
	virtual Instance::RequestMethod getRequestMethod() { return Instance::Get; }

	// The function called to response to a matched request
	virtual HTTPResponseWrapper::Ptr getPage(const HTTPRequest& request) = 0;

};


class PageManager {
public:

	void registerRoutes(Instance& instance);

	void insertPage(shared_ptr<Page> page);

private:

	static HTTPResponseWrapper::Ptr routeHandler(shared_ptr<Page> page, const HTTPRequest& request);

	vector<shared_ptr<Page>> pages;
};

extern PageManager pageManager;

