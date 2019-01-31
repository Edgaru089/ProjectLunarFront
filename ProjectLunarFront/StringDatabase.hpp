#pragma once

#include "Main.hpp"
#include "Lockable.hpp"

class StringDBObject :public enable_shared_from_this<StringDBObject> {
public:

	void setId(int id) { this->id = id; }
	int& getId() { return id; }
	const int& getId() const { return id; }

	void setFileDir(const wstring& dir) { filedir = dir; }
	const wstring& getFileDir() const { return filedir; }

	bool isLoaded() const { return loaded; }
	void ensureLoaded() const { if (!loaded) { contents = readFileBinary(filedir); loaded = true; } }

private:
	int id;
	wstring filedir;
	mutable string contents;
	mutable bool loaded;
};

class StringDatabase :public Lockable {
public:
private:
	int maxId;
};
