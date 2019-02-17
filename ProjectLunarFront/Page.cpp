
#include "Page.hpp"

#include "Config.hpp"

PageManager pageManager;


void PageManager::registerRoutes(Instance& instance) {
	for (auto p : pages)
		instance.registerRouteRule(
			p->getRequestPrefix(),
			config.getHostname(),
			bind(PageManager::routeHandler, p, placeholders::_1),
			p->getRequestMethod());
}


void PageManager::insertPage(shared_ptr<Page> page) {
	pages.push_back(page);
}


HTTPResponseWrapper::Ptr PageManager::routeHandler(shared_ptr<Page> page, const HTTPRequest& request) {
	return page->getPage(request);
}

