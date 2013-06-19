#include <iostream>
#include <string>
#include <unistd.h>
#include <curl/curl.h>
#include "HttpReader.h"
#include "RssParser.h"
#include "FileManager.h"
#include "ViewManager.h"

using namespace std;


void HandleArguments(int argc, char *argv[]);
void AddSource(string source);
void RemoveSource(string source);
void ListSources();
void PrintHelp();


int main(int argc, char *argv[]) {
	if (argc > 1) {
		HandleArguments(argc, argv);	
		return 0;
	}

	ViewManager viewMgr;
	viewMgr.MainLoop();  
	
    return 0;
}


void HandleArguments(int argc, char *argv[]) {
	int i=1;
	while (i < argc) {
		if (strcmp(argv[i], "-a") == 0) {
			if (argc > i+1) {
				AddSource(argv[++i]);
			} else {
				PrintHelp();
			}
		}

		if (strcmp(argv[i], "-r") == 0) {
			if (argc > i+1) {
				RemoveSource(argv[++i]);
			} else {
				PrintHelp();
			}
		} 

		if (strcmp(argv[i], "-l") == 0) {
			ListSources();
		}

		if (strcmp(argv[i], "-h") == 0) {
			PrintHelp();
		}

		i++;
	}
}


void AddSource(string source) {
	FileManager::AddSource(source);
}


void RemoveSource(string source) {
	FileManager::RemoveSource(source);
}


void ListSources() {
	vector< pair<string,string> > regs;
	regs = FileManager::GetRegisteredSources();

	for (int i=0; i<regs.size(); i++) {
		cout << regs[i].second << "\n";
	}
}


void PrintHelp() {
	static bool hasDisplayedHelp = false;

	if (!hasDisplayedHelp) {
		cout<< "-a [RSS-url]\n"
			<< "\tAdd the specified RSS-feed\n"
			<< "-r [(partial) RSS-url]\n"
			<< "\tThe URL does not have to match completely. If you are tracking\n"
			<< "\tthe source 'http://wwww.superfeed.com/rss', 'superfeed' will suffice.\n"
			<< "-l\n"
			<< "\tList all tracked sources\n"
			<< "-h\n"
			<< "\tDisplay this text\n";
	}

	hasDisplayedHelp = true;
}
