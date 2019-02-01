#pragma once

#include "Main.hpp"
#include "Lockable.hpp"

class StringDBObject :public enable_shared_from_this<StringDBObject> {
public:
	StringDBObject() :loaded(false) {}
	StringDBObject(Uuid id, const fs::path& file) :
		id(id), filedir(file), loaded(false) {}
	StringDBObject(Uuid id, const fs::path& file, const string& contents) :
		id(id), filedir(file), contents(contents), loaded(true) {}

	Uuid getId() const { return id; }
	const fs::path& getFileDir() const { return filedir; }
	uintmax_t getSize() { return fs::file_size(filedir); }

	bool isLoaded() const { return loaded; }
	void ensureLoaded() const {
		if (!loaded) {
			contents = readFileBinary(filedir.generic_wstring());
			if (!contents.empty())
				loaded = true;
		}
	}

	const string& getString() const { ensureLoaded(); return contents; }

	bool isValid() { return id != Uuid::nil(); }

private:
	friend class StringDatabase;

	Uuid id;
	fs::path filedir;
	mutable string contents;
	mutable bool loaded;
};

class StringDatabase :public Lockable {
public:

	bool initializeWithFolder(const fs::path& dbFolder);

	Uuid insert(const string& contents);
	bool remove(Uuid id);

	const StringDBObject& get(Uuid id) {
		lock_guard<Lockable>(*this);
		auto i = objs.find(id);
		if (i == objs.end())
			return empty;
		else
			return i->second;
	}

private:
	fs::path dbdir;
	map<Uuid, StringDBObject> objs;
	StringDBObject empty;
};

extern StringDatabase stringDatabase;

