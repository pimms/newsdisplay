#pragma once
#include <string>

using namespace std;

class HttpReader {
public:
					HttpReader(string url);
					~HttpReader(void);
	
	bool			Perform();
	string&			GetContents();

private:
	string			sourceUrl;
	string			contents;
};

