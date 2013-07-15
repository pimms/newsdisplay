#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "tinyxml.h"

using namespace std;

class HttpReader;


template<class C>
bool IsItemContained(const vector<C> &vec, const C &item);


struct RssItem {
	string			title;
	string			link;
	string			desc;
	time_t			timestamp;
	
					RssItem();
	void			WriteToFile(fstream &file);

					// return true:	
					//	    The object read in at least one valid value
					// return false:
					//	    No data was read, the object should be discarded.
	bool			ReadFromFile(fstream &file);

	bool 			TooOld();

	bool			operator==(const RssItem &o) const;
	bool			operator!=(const RssItem &o) const;

					// Operator for std::sort algorithm
	bool 			operator<(const RssItem &o) const;
};


class RssParser
{
public:
	static string	RemoveHTML(string str);
	static string	RemoveNewlines(string str);

					RssParser(string url, vector<RssItem> *items);
					~RssParser(void);

					// Return values:
					// -2      Invalid XML (site is not an RSS-feed)
					// -1      Failed to retrieve web content
					// 0-inf   Count of new items
	int				Perform();
	string			GetSource();

private:
	string			source;
	HttpReader		*httpReader;
	vector<RssItem> *rssItems;

	TiXmlDocument	*xmlDoc;
	TiXmlElement	*elemRss;	// The root of the document
	TiXmlElement	*elemChan;	
	TiXmlElement	*elemItem;	// The LAST item taken care of

	bool			SetRssRootNode();
	bool			SetNextChannel();
	bool			SetNextItem();
	int				HandleCurrentItem();
};


class ItemManager {
public:
					ItemManager(string sourceUrl);
					~ItemManager();

					// Return values:
					// -1      Failed to retrieve from remote source
					// 0-inf   The amount of new items
	int 			Reload();
	string			GetSource();

	vector<RssItem> rssItems;

private:
	RssParser		*rssParser;

	void			WriteToFile();
	void			LoadFromFile();
	int				LoadFromSource();

	void 			SortByDate();
};