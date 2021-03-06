
#include "Main.hpp"

#include "BuildCounter.hpp"

#define PROJECT_NAME "Project LunarFront"
#define PROJECT_STAGE "Alpha"
#define _FULL_SERVER_NAME(build) "Project-LunarFront/0.0.0.-Alpha"
#define FULL_SERVER_NAME _FULL_SERVER_NAME(BUILD)

#define MAJORVER 0
#define MINORVER 0
#define PATCHVER 0
#define BUILDVER BUILD


const char projectName[] = PROJECT_NAME, stage[] = PROJECT_STAGE;
const int majorVersion = MAJORVER, minorVersion = MINORVER, patchVersion = PATCHVER, buildVersion = BUILDVER;

const char completeServerName[] = FULL_SERVER_NAME;
const string completeServerNameEx = StringParser::toStringF(PROJECT_NAME " " PROJECT_STAGE " Version %d.%d.%d Build %d", majorVersion, minorVersion, patchVersion, buildVersion);

Config config;

// from Uuid.hpp
mt19937 uuid_random_engine;
uniform_int_distribution<unsigned int> uuid_distribution(0, UINT_MAX);

namespace {

	mt19937 randomEngine((random_device())());

	char decodeHex2(char high, char low) {
		return (unsigned char)((((high >= 'A') ? (high - 'A' + 10) : (high - '0')) << 4) | ((low >= 'A') ? (low - 'A' + 10) : (low - '0')));
	}

	char encodeHex1(unsigned char c) {
		return (c >= 0xa) ? ('A' + c - 0xa) : ('0' + c);
	}

	pair<char, char> encodeHex2(unsigned char c) {
		return make_pair(encodeHex1((c & 0xf0) >> 4),
						 encodeHex1(c & 0xf));
	}

	double rand01() {
		return uniform_real_distribution<double>(0.0, 1.0)(randomEngine);
	}

	// [x, y]
	int rand(int x, int y) {
		return uniform_int_distribution<int>(x, y)(randomEngine);
	}

	// A cache container for readFileBinaryCached
	map<wstring, pair<chrono::steady_clock::time_point, string>> fileCache;
	const auto reloadCachedFileDuration = chrono::seconds(3);
	string cachedEmptyString;
}

#ifdef _WIN32

string wstringToUtf8(const wstring& source) {
	size_t size = WideCharToMultiByte(CP_UTF8, 0, source.data(), source.size(), NULL, NULL, NULL, NULL);
	string buffer(size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, source.data(), source.size(), buffer.data(), buffer.size(), NULL, NULL);
	return buffer;
}

wstring utf8ToWstring(const string& source) {
	size_t size = MultiByteToWideChar(CP_UTF8, 0, source.data(), source.size(), NULL, NULL);
	wstring buffer(size, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, source.data(), source.size(), buffer.data(), buffer.size());
	return buffer;
}

wstring ansiToWstring(const string& source) {
	size_t size = MultiByteToWideChar(CP_ACP, 0, source.data(), source.size(), NULL, NULL);
	wstring buffer(size, L'\0');
	MultiByteToWideChar(CP_ACP, 0, source.data(), source.size(), buffer.data(), buffer.size());
	return buffer;
}

#else

string wstringToUtf8(const wstring& source) {
	// In C++11+, std::string stores a null-terminated string. Funtcion string::c_str() returns the beginning of the array.
	mbstate_t state = mbstate_t();
	const wchar_t* src = source.c_str();
	size_t size = wcsrtombs(nullptr, &src, 0, &state);
	string buffer(size, '\0');
	wcsrtombs(buffer.data(), &src, buffer.size() + 1, &state);
	return buffer;
}

wstring utf8ToWstring(const string& source) {
	// HACK Assuming ANSI == UTF-8 on non-Windows platforms
	mbstate_t state = mbstate_t();
	const char* src = source.c_str();
	size_t size = mbsrtowcs(nullptr, &src, 0, &state);
	wstring buffer(size, L'\0');
	mbsrtowcs(buffer.data(), &src, buffer.size() + 1, &state);
	return buffer;
}

wstring ansiToWstring(const string& source) {
	// HACK Assuming ANSI == UTF-8 on non-Windows platforms
	return utf8ToWstring(source);
}

#endif

string decodePercentEncoding(const string& source) {
	string res;
	int i = 0;
	while (i < source.length()) {
		if (source[i++] == '%') {
			char c1 = source[i++];
			char c2 = source[i++];
			res.push_back(decodeHex2(c1, c2));
		} else if (source[i - 1] == '+')
			res.push_back(' ');
		else
			res.push_back(source[i - 1]);
	}
	return res;
}

string encodePercent(const string& source, bool encodeSlash) {
	string res;
	for (unsigned char i : source) {
		if (isalnum(i) || i == '.' || i == '-' || i == '_' || i == '~' || (!encodeSlash&&i == '/'))
			res.push_back(i);
		else {
			res.push_back('%');
			auto&&[c1, c2] = encodeHex2(i);
			res.push_back(c1);
			res.push_back(c2);
		}
	}
	return res;
}

void decodeFormUrlEncoded(string body, map<string, string>& result) {
	int i = 0;
	while (i < body.size()) {
		pair<string, string> cur;
		while (body[i] == '&') i++;
		while (body[i] != '=')
			cur.first.push_back(body[i++]);
		while (body[i] == '=') i++;
		while (i < body.size() && body[i] != '&')
			cur.second.push_back(body[i++]);
		result.insert(make_pair(decodePercentEncoding(cur.first), decodePercentEncoding(cur.second)));
	}
}

string encodeCookieSequence(const vector<pair<string, string>>& cookies) {
	string ans;
	for (auto&[id, body] : cookies) {
		if (!ans.empty())
			ans += "; ";
		ans += id + '=' + body;
	}
	return ans;
}

map<string, string> decodeCookieSequence(string body) {
	map<string, string> ans;

	int i = 0;
	while (i < body.size()) {
		pair<string, string> cur;
		while (body[i] == ';') i++;
		while (body[i] == ' ') i++;
		while (body[i] != '=')
			cur.first.push_back(body[i++]);
		while (body[i] == '=') i++;
		while (i < body.size() && body[i] != ';')
			cur.second.push_back(body[i++]);
		ans.insert(make_pair(decodePercentEncoding(cur.first), decodePercentEncoding(cur.second)));
	}

	return ans;
}

void decodeCookieAndUriParam(const HTTPRequest& request, map<string, string>& cookies, map<string, string>& uriParams) {
	const string& uri = request.GetURI();
	if (size_t pos = uri.find_first_of('?'); pos != string::npos) {
	// has a uri param-list, parse it
		decodeFormUrlEncoded(uri.substr(pos + 1), uriParams);
	}
	decodeCookieSequence(request.GetHeaderValue("Cookie"));
}

string readFileText(const wstring& filename) {
	ifstream file;
	OPEN_FSTREAM_WSTR(file, filename, ifstream::in);
	if (!file)
		return string();

	// Get the file size
	file.ignore(numeric_limits<streamsize>::max());
	size_t fileSize = (size_t)file.gcount();
	file.seekg(0, ifstream::beg);

	// Read
	string res(fileSize, '\0');
	file.read(res.data(), fileSize);
	return res;
}

string readFileBinary(const wstring& filename) {
	ifstream file;
	OPEN_FSTREAM_WSTR(file, filename, ifstream::binary);
	if (!file)
		return string();

	// Get the file size
	file.ignore(numeric_limits<streamsize>::max());
	size_t fileSize = (size_t)file.gcount();
	file.seekg(0, ifstream::beg);

	// Read
	string res(fileSize, '\0');
	file.read(res.data(), fileSize);
	return res;
}

const string& readFileBinaryCached(const wstring& filename) {
	auto i = fileCache.find(filename);
	if (i == fileCache.end() || chrono::steady_clock::now() - i->second.first > reloadCachedFileDuration) {
		// Load or reload the file
		if (i == fileCache.end())
			i = fileCache.insert(make_pair(filename, make_pair(chrono::steady_clock::now(), ""))).first;
		else
			i->second.first = chrono::steady_clock::now();

		ifstream file;
		OPEN_FSTREAM_WSTR(file, filename, ifstream::binary);
		if (!file)
			return cachedEmptyString;

		// Get the file size
		file.ignore(numeric_limits<streamsize>::max());
		size_t fileSize = (size_t)file.gcount();
		file.seekg(0, ifstream::beg);

		string& res = i->second.second;
		res.clear(); res.resize(fileSize); res.shrink_to_fit();
		file.read(res.data(), fileSize);
	}
	return i->second.second;
}

bool writeFileText(const wstring& filename, const string& contents) {
	ofstream fout;
	OPEN_FSTREAM_WSTR(fout, filename, ofstream::out);
	if (!fout)
		return false;
	fout.write(contents.c_str(), contents.size());
	fout.flush();
	fout.close();
	return true;
}

bool writeFileBinary(const wstring& filename, const string& contents) {
	ofstream fout;
	OPEN_FSTREAM_WSTR(fout, filename, ofstream::binary);
	if (!fout)
		return false;
	fout.write(contents.c_str(), contents.size());
	fout.flush();
	fout.close();
	return true;
}

string toUppercase(const string& str) {
	string ans;
	for (char c : str) {
		if (c > 0)
			ans.push_back(toupper(c));
		else
			ans.push_back(c);
	}
	return ans;
}

string generateCookie(int length) {
	string str;
	str.reserve(length);
	for (int i = 1; i <= length; i++) {
		if (rand(0, 1)) // Uppercase
			str.push_back('A' + rand(0, 25));
		else // Lowercase
			str.push_back('a' + rand(0, 25));
	}
	return str;
}

string getUserNavString(const map<string, string>& cookies) {
	// TODO: getUserNavString()
	return "TODO: getUserNavString()";
}

string escapeTextHTML(const string& source) {
	return StringParser::replaceSubString(source, {
		{"&","&amp;"},
		{"\"","&quot;"},
		{"\'","&apos;"},
		{"<","&lt;"}, {">","&gt;"}, {"!","&excl;"}, {"\t","    "}
	});
}

