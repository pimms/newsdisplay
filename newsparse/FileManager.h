#pragma once

#include <vector>
#include <fstream>

#include "RssParser.h"

using namespace std;

/*
	Registry file:
		[filename 1]
		[feed URL 1]
		[filename 2]
		[feed URL 2]
		...
		
	RSS-cache:
		[ITEM]
		[TITLE]
		Super-RSS feed!
		[LINK]
		www.super-rss.com
		[DESC]
		This is like the total kickass & stuff...!!!!
		..... 
*/

class FileManager {
public:
	static string	FilenameFromSource(string source);
	static vector< pair<string,string> >
					GetRegisteredSources();
	static bool		AddSource(string source);
	static bool		RemoveSource(string source);

					FileManager();
					~FileManager();

	void			WriteToFile(vector<RssItem> *items, string source);
	void			ReadFromFile(vector<RssItem> *items, string source);

private:
	static bool		OpenFile(fstream &file, string filename, int mode, bool create);
	static void		CreateDummyFile(string filename);
	static void		WriteRegistry(vector< pair<string,string> > regs);

	static void		RegisterSource(string source, string file);
	static bool		IsSourceRegistered(string source);
	static string	GetRegisteredSourceFile(string source);

	void			ResetHandle(fstream &file);
};

