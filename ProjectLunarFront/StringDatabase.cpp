
#include "StringDatabase.hpp"

StringDatabase stringDatabase;


bool StringDatabase::initializeWithFolder(const fs::path& dbFolder) {
	if (!fs::is_directory(dbFolder)) {
		fs::remove(dbFolder);
		fs::create_directory(dbFolder);
	}

	lock();
	mlog << "[StringDatabase] Initializing, folder: " << dbFolder.generic_wstring() << dlog;
	dbdir = dbFolder;

	for (const auto& f : fs::directory_iterator(dbFolder)) {
		if (f.is_regular_file()) {
			Uuid id(f.path().filename().generic_u8string());
			if (id == Uuid::nil())
				continue;
			objs.insert(make_pair(id, StringDBObject(id, f.path())));
			mlog << "                 Loaded object {" << id.toString() << "} with length=" << f.file_size() << dlog;
		}
	}

	mlog << "[StringDatabase] Initialization done, objects: " << objs.size() << dlog;
	unlock();
	return true;
}


bool StringDatabase::remove(Uuid id) {
	throw NotImplementedException("StringDatabase::remove");
}


Uuid StringDatabase::insert(const string& contents) {
	Uuid id = Uuid::get();
	string str = id.toString();

	mlog << "[StringDatabase] Insert string object with id=" << str << dlog;

	fs::path newfile = dbdir / str;
	writeFileBinary(newfile.generic_wstring(), contents);

	lock();
	objs.insert(make_pair(id, StringDBObject(id, newfile, contents)));
	unlock();

	return id;
}

