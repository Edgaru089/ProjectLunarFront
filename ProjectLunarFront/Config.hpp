#pragma once

#include <string>
#include <fstream>
#include <map>
#include <mutex>
#include <filesystem>
#include "Main.hpp"

using namespace std;

class OptionFile {
public:

	bool loadFromFile(const wstring& filename)
	{
		string line, mark, cont;
		this->data.clear();
		ifstream file;
		OPEN_FSTREAM_WSTR(file, filename, ifstream::in);
		if (file.fail())
			return false;
		while (!file.eof()) {
			getline(file, line);
			if (line[0] == '#')
				continue;
			size_t pos = line.find('=');
			if (pos == string::npos)
				continue;
			mark = line.substr(0, pos);
			cont = line.substr(pos + 1, line.length() - pos - 1);
			if (cont.back() == '\r') // Windows line-ending
				cont.pop_back();
			this->data[mark] = cont;
		}
		return true;
	}

	string getContent(string key) {
		map<string, string>::iterator i;
		if ((i = this->data.find(key)) != this->data.end()) {
			return i->second;
		} else
			return "";
	}

private:
	map<string, string> data;
};

class Config {
public:

	bool loadFromFile(const wstring& filename) {
		if (!file.loadFromFile(filename))
			return false;

		string listenPortStr = "5000";

		setStringIfNotEmpty(tempDir, file.getContent("temp_dir"));
		setStringIfNotEmpty(listenPortStr, file.getContent("port"));
		setStringIfNotEmpty(hostname, file.getContent("hostname"));
		setStringIfNotEmpty(cppCompiler, file.getContent("cpp_compiler"));

		// if tempDir does not exist, create it
		filesystem::create_directories(tempDir);

		return true;
	}

	const string& getTempDir() { return tempDir; }
	const string& getHostname() { return hostname; }
	const string& getCppCompiler() { return cppCompiler; }
	int getListenPort() { return listenPort; }

private:

	void setStringIfNotEmpty(string& target, const string& source) {
		if (!source.empty())
			target = source;
	}

	OptionFile file;

	string tempDir = "temp/";
	string hostname = "127.0.0.1:5000";
	string cppCompiler = "g++ %CODE -o %EXE";
	int listenPort;
};

