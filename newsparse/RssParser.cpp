#include "RssParser.h"
#include "HttpReader.h"
#include "FileManager.h"

#include "tinyxml.h"
#include <fstream>
#include <ctime>


template<class C>
bool IsItemContained(const vector<C> &vec, const C &item) {
	for (int i=0; i<vec.size(); i++) {
		if (vec[i] == item) {
			return true;
		}
	}
	return false;
}


// ==========================================================


RssItem::RssItem() {
	time = 0;
}


void RssItem::WriteToFile(fstream &file) {
	file << "[ITEM]\n";

	if (title.length()) {
		file << "[TITLE]\n" << title << "\n";
	}

	if (link.length()) {
		file << "[LINK]\n" << link << "\n";
	}

	if (desc.length()) {
		file << "[DESC]\n" << desc << "\n";
	}

	file << "[TIME]\n" << time << "\n";
}


bool RssItem::ReadFromFile(fstream &file) {
	string line;
	int hits = 0;

	while (file) {
		getline(file, line, '\n');

		if (line == "[ITEM]") {
			break;
		}

		if (line == "[TITLE]") {
			getline(file, title, '\n');
			hits++;
		}

		if (line == "[LINK]") {
			getline(file, link, '\n');
			hits++;
		}

		if (line == "[DESC]") {
			getline(file, desc, '\n');
			hits++;
		}

		if (line == "[TIME]") {
			file >> time; file.ignore();
		}
	}

	return hits != 0;
}


bool RssItem::operator==(const RssItem &o) const  {
	return	title == o.title &&
			link == o.link &&
			desc == o.desc;
}


bool RssItem::operator!=(const RssItem &o) const  {
	return !(*this == o);
}


// ==========================================================


string RssParser::RemoveHTML(string str) {
	bool insideTag = false;
	string result;

	for (int i=0; i<str.length(); i++) {
		if (str[i] == '<') {
			insideTag = true;
		}
		
		if (!insideTag) {
			result += str[i];
		}

		if (str[i] == '>') {
			insideTag = false;
		}
	}

	return result;
}


string RssParser::RemoveNewlines(string str) {
	for (int i=0; i<str.length(); i++) {
		if (str[i] == '\n') {
			str[i] = ' ';
		}
	}

	return str;
}


RssParser::RssParser(string url, vector<RssItem> *items) {
	httpReader = new HttpReader(url);
	source = url;
	rssItems = items;

	xmlDoc = NULL;
	elemRss = NULL;
	elemChan = NULL;
	elemItem = NULL;
}


RssParser::~RssParser(void) {
	delete httpReader;

	if (xmlDoc) {
		delete xmlDoc;
	}
}


int RssParser::Perform() {
	if (!httpReader->Perform()) {
		printf("Failed to retrieve web content\n");
		return -1;
	}

	if (xmlDoc) {
		delete xmlDoc;
	}
	
	xmlDoc = new TiXmlDocument;
	xmlDoc->Parse(httpReader->GetContents().c_str());

	if (!SetRssRootNode()) {
		printf("Invalid XML - could not find root node <rss>!\n");
		return -2;
	}

	int num = 0;

	while (SetNextChannel()) {
		while (SetNextItem()) {
			num += HandleCurrentItem();
		}
	}

	return num;
}


string RssParser::GetSource() {
	return source;
}




bool RssParser::SetRssRootNode() {
	if (!xmlDoc) {
		return false;
	}

	elemRss = xmlDoc->FirstChildElement("rss");

	return (bool)elemRss;
}


bool RssParser::SetNextChannel() {
	if (!elemRss) {
		printf("No RSS-node defined!\n");
		return false;
	}

	if (!elemChan) {
		elemChan = elemRss->FirstChildElement("channel");
	} else {
		elemChan = elemChan->NextSiblingElement("channel");
	}

	return (bool)elemChan;
}


bool RssParser::SetNextItem() {
	if (!elemChan) {
		printf("No channel defined!\n");
		return false;
	}

	if (!elemItem) {
		elemItem = elemChan->FirstChildElement("item");
	} else {
		elemItem = elemItem->NextSiblingElement("item");
	}

	return (bool)elemItem;
}


int RssParser::HandleCurrentItem() {
	if (!elemItem) {
		printf("No item-element defined!\n");
		return 0;
	}

	TiXmlElement *elem = NULL;
	RssItem item;
	int fields = 0;

	elem = elemItem->FirstChildElement("title");
	if (elem) {
		item.title = RemoveNewlines(RemoveHTML(elem->GetText()));
		fields++;
	}
	
	elem = elemItem->FirstChildElement("link");
	if (elem) {
		item.link = RemoveNewlines(RemoveHTML(elem->GetText()));
		fields++;
	}

	elem = elemItem->FirstChildElement("description");
	if (elem) {
		item.desc = RemoveNewlines(RemoveHTML(elem->GetText()));
		fields++;
	}

	if (fields) {
		if (!IsItemContained<RssItem>(*rssItems, item)) {
			item.time = time(0);
			rssItems->push_back(item);
			return 1;
		}
	}

	return 0;
}


// ==========================================================


ItemManager::ItemManager(string sourceUrl) {
	rssParser = new RssParser(sourceUrl, &rssItems);
}


ItemManager::~ItemManager() {
	delete rssParser;
}


int ItemManager::Reload() {
	rssItems.clear();
	LoadFromFile();

	int newSources = LoadFromSource();
	if (newSources) {
		WriteToFile();
	}

	return newSources;
}


string ItemManager::GetSource() {
	return rssParser->GetSource();
}




void ItemManager::WriteToFile() {
	FileManager fmgr;
	fmgr.WriteToFile(&rssItems, rssParser->GetSource());
}


void ItemManager::LoadFromFile() {
	FileManager fmgr;
	fmgr.ReadFromFile(&rssItems, rssParser->GetSource());
}


int ItemManager::LoadFromSource() {
	return rssParser->Perform();
}