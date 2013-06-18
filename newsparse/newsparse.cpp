#include <iostream>
#include <string>
#include <curl/curl.h>
#include "HttpReader.h"
#include "RssParser.h"
#include "FileManager.h"

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

    curl_global_init(CURL_GLOBAL_ALL);

	vector< pair<string,string> > reg;
	reg = FileManager::GetRegisteredSources();

	vector<ItemManager*> itemMgrs;

	for (int i=0; i<reg.size(); i++) {
		string file, source;
		file = reg[i].first;
		source = reg[i].second;

		ItemManager *mgr = new ItemManager(source);
		itemMgrs.push_back(mgr);
	}

	for (int i=0; i<itemMgrs.size(); i++) {
		int countNew = itemMgrs[i]->Reload();
		int count = itemMgrs[i]->rssItems.size();

		cout << countNew << " new feeds from " << itemMgrs[i]->GetSource()  << ":\n";

		for (int j=count-countNew; j<count; j++) {
			RssItem *item = &itemMgrs[i]->rssItems[j];
			
			cout << item->time << "\n";
			cout << item->title << "\n";
			cout << item->desc << "\n";
			cout << item->link << "\n";
			cout << "\n";
		}
	}

	curl_global_cleanup();
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
	vector< pair<string,string> > regs = FileManager::GetRegisteredSources();

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