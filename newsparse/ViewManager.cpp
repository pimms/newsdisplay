#include "ViewManager.h"
#include "FileManager.h"
#include "RssParser.h"

#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <curl/curl.h>

#include <atomic>

#define REFRESH_INTERVAL 	60
#define SUBVIEW_W    	 	40
#define SUBVIEW_H 	 		7


pthread_mutex_t refreshlock;
bool undrawnItems = false;
volatile bool refreshRunning = false;

void* Refresh(void *param) {
	refreshRunning = true;
	ViewManager *viewMgr = (ViewManager*)param;

	pthread_mutex_lock(&refreshlock);

	curl_global_init(CURL_GLOBAL_ALL);
	viewMgr->ReloadItems();
	curl_global_cleanup();

	pthread_mutex_unlock(&refreshlock);

	refreshRunning = false;
	return NULL;
}


// =============================================


pthread_mutex_t inputlock;
string g_input;
bool t_continue = true;

void StdinNoncanonical(struct termios &orgopt) {
	struct termios new_opts;

	tcgetattr(STDIN_FILENO, &orgopt);
	memcpy(&new_opts, &orgopt, sizeof(new_opts));

	new_opts.c_lflag &= ~(ECHO);
	new_opts.c_lflag &= ~(ICANON);
	new_opts.c_cc[VMIN] = 0;
	new_opts.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
}

void StdinCanonical(struct termios &orgopt) {
	tcsetattr(STDIN_FILENO, TCSANOW, &orgopt);
}

void* StdinThread(void*) {
	struct termios orgopts;
	int c=0, res=0;

	StdinNoncanonical(orgopts);

	while (t_continue) {
		while ((c = getchar()) > 0) {
			pthread_mutex_lock(&inputlock);
			g_input += c;
			pthread_mutex_unlock(&inputlock);
		}
		
		usleep(10000);
	}

  	StdinCanonical(orgopts);

  	return NULL;
}


// =============================================


bool Subview::colPairsInit = false;

Subview::Subview() {
	if (!colPairsInit && has_colors()) {
		for (int i=1; i<=7; i++) {
			init_pair(i, i, 0);
		}

		init_pair(8, COLOR_BLACK, COLOR_WHITE);

		colPairsInit = true;
	}
}


int Subview::GetColorPair(string title, string desc) {
	long long int total = 0;

	for (int i=0; i<title.length(); i++) {
		total += title[i];
	}

	for (int i=0; i<desc.length(); i++) {
		total += desc[i];
	}

	return (total % 7) + 1;
}


// =============================================


ViewManager::ViewManager() {
	initscr();

	if (has_colors()) {
		start_color();
	}

	selectedIndex = 0;
	numX = 0;
	numY = 0;
	dimx = 0;
	dimy = 0;
}


ViewManager::~ViewManager() {
	ClearManagers();
	endwin();
}


void ViewManager::MainLoop() {
	bool enableInput = true;
	pthread_t inputThread, refreshThread;

	if (enableInput &&
		pthread_mutex_init(&inputlock, NULL)) {
		enableInput = false;
	}

	if (enableInput && 
		pthread_mutex_init(&refreshlock, NULL)) {
		enableInput = false;
	}

	if (enableInput && 
		pthread_create(&inputThread, NULL, &StdinThread, NULL)) {
		enableInput = false;
	}

	time_t nextUpdate = time(0) + REFRESH_INTERVAL;
	ReloadItems();
	Redraw();

	while (true) {
		if (enableInput && !refreshRunning) {
			HandleInput();
		}

		time_t now = time(0);
		if (now >= nextUpdate) {
			if (enableInput) {
				if (!refreshRunning) {
					pthread_create(&refreshThread, NULL, 
								   &Refresh, this);
				}
			} else {
				ReloadItems();
				Redraw();
			}
			nextUpdate = now + REFRESH_INTERVAL;
		} else {
			usleep(10000);
		}

		if (pthread_mutex_trylock(&refreshlock)) {
			if (undrawnItems && !refreshRunning) {
				Redraw();
				undrawnItems = false;
			}
			pthread_mutex_unlock(&refreshlock);
		}
	}
}


void ViewManager::Redraw() {
	erase();

	CalculateDimensions();
	AssignDisplayItems(subviews.size());

	for (int i=0; i<subviews.size(); i++) {
		DrawSubview(i);
	}

	DrawEdges();
	refresh();
}


void ViewManager::ReloadItems() {
	vector< pair<string,string> > regs;
	regs = FileManager::GetRegisteredSources();

	ClearManagers();
	undrawnItems = true;

	for (int i=0; i<regs.size(); i++) {
		string source = regs[i].second;

		ItemManager *mgr = new ItemManager(source);
		
		int result = mgr->Reload();

		if (result >= 0) {
			itemMgrs.push_back(mgr);
		} else {
			delete mgr;
		}
	}
}




void ViewManager::HandleInput() {
	pthread_mutex_lock(&inputlock);

	if (g_input.length()) {
		int prevIndex = selectedIndex;

		for (int i=0; i<g_input.length(); i++) {
			char c = g_input[i];

			/* React to arrow presses */
			if (c == 65) {
				selectedIndex -= numX;
			}
			if (c == 66) {
				selectedIndex += numX;
			}
			if (c == 67) {
				selectedIndex++;
			}
			if (c == 68) {
				selectedIndex--;
			}

			/* Clamp the index */
			if (selectedIndex >= subviews.size()) {
				selectedIndex -= subviews.size();
			}
			if (selectedIndex < 0) {
				selectedIndex += subviews.size();
			}

			/* Follow the items link on enter */
			if (c == 13) {
				if (selectedIndex < displayItems.size()) {
					RssItem *item = displayItems[selectedIndex];
					OpenLink(item->link);
				}
			}
		}

		if (prevIndex != selectedIndex) {
			DrawSubview(prevIndex, 7);
			DrawSubview(selectedIndex, 8);
			refresh();
		}

		g_input.clear();
	}

	pthread_mutex_unlock(&inputlock);
}


void ViewManager::CalculateDimensions() {
	getmaxyx(stdscr, dimy, dimx);
	subviews.clear();

	// Subviews draw their top and left edge.
	// The rightmost and bottommost edges are
	// not allocated to subviews, but drawn by
	// itself.
	numX = (dimx-1) / SUBVIEW_W;
	numY = (dimy-1) / SUBVIEW_H;

	int usedX = 0;
	int usedY = 0;


	int excessY = (dimy-1) - numY * SUBVIEW_H;
	for (int y=0; y<numY; y++) {
		int extraY = excessY / (numY - y);
		excessY -= extraY;

		int excessX = (dimx-1) - numX * SUBVIEW_W;
		for (int x=0; x<numX; x++) {		
			int extraX = excessX / (numX - x);
			excessX -= extraX;

			Subview sub;

			sub.x = usedX;
			sub.y = usedY;

			sub.w = SUBVIEW_W + extraX;
			sub.h = SUBVIEW_H + extraY;

			subviews.push_back(sub);

			usedX += sub.w;
		}

		usedY += SUBVIEW_H + extraY;
		usedX = 0;
	}
}	


void ViewManager::AssignDisplayItems(int count) {
	displayItems.clear();

	// Shove all items into a vector 
	vector<RssItem*> items;

	for (int i=0; i<itemMgrs.size(); i++) {
		ItemManager *mgr = itemMgrs[i];
		for (int j=0; j<mgr->rssItems.size(); j++) {
			items.push_back(&mgr->rssItems[j]);
		}
	}

	// Find as many new as possible, ordered from
	// newest to oldest
	while (items.size() && displayItems.size() < count) {
		RssItem *item = items[0];
		int bestIdx = 0;

		for (int i=1; i<items.size(); i++) {
			if (items[i]->time > item->time) {
				item = items[i];
				bestIdx = i;
			}
		}

		items.erase(items.begin() + bestIdx);
		displayItems.push_back(item);
	}
}


void ViewManager::DrawSubview(int idx, int fillColorPair) {
	Subview *sub = &subviews[idx];
	RssItem *item = NULL;

	// Draw the edges
	
	for (int x=sub->x; x<sub->x+sub->w; x++) {
		mvaddch(sub->y, x, '#');
	}

	for (int y=sub->y; y<sub->y+sub->h; y++) {
		mvaddch(y, sub->x, '#');
	}

	// Clear the subview
	FillSubview(idx, fillColorPair);

	// draw the text within the subview, if
	// enough items are available
	if (idx < displayItems.size()) {
		item = displayItems[idx];
	} else {
		return;
	}

	vector<string> titleLines, titleDesc;
	DivideString(item->title, titleLines, sub->w-3);
	DivideString(item->desc, titleDesc, sub->w-3);

	int line = 1;
	int totalLines = titleLines.size() + titleDesc.size() + 1;

	
	int pair = sub->GetColorPair(item->title, item->desc);

	if (has_colors()) {
		if (idx == selectedIndex) {
			pair = 8;
		} 

		attron(COLOR_PAIR(pair));
	}

	while (line < sub->h) {
		vector<string> *vec = (titleLines.size()) 
							   ? &titleLines
							   : (titleDesc.size())
							   	  ? &titleDesc
							   	  : NULL;
		if (vec) {
			string str = (*vec)[0];
			vec->erase(vec->begin());

			int x = sub->x + sub->w/2 - str.length()/2;
			int y = sub->y + line;

			mvaddstr(y, x, str.c_str());
			line++;

			if (vec == &titleLines && !vec->size()) {
				line++;
			}
		} else {
			break;
		}
	}

	attroff(COLOR_PAIR(pair));
}


void ViewManager::DrawEdges() {
	for (int x=0; x<dimx; x++) {
		mvaddch(dimy-1, x, '#');
	}

	for (int y=0; y<dimy; y++) {
		mvaddch(y, dimx-1, '#');
	}
}


void ViewManager::FillSubview(int subIdx, int colorPair) {
	if (has_colors()) {
		attron(COLOR_PAIR(colorPair));

		Subview *sub = &subviews[subIdx];
		for (int x=sub->x+1; x<sub->x+sub->w; x++) {
			for (int y=sub->y+1; y<sub->y+sub->h; y++) {
				mvaddch(y, x, ' ');
			}
		}

		attroff(COLOR_PAIR(colorPair));
	}
}


void ViewManager::ClearManagers() {
	for (int i=0; i<itemMgrs.size(); i++) {
		delete itemMgrs[i];
	}

	displayItems.clear();
	itemMgrs.clear();
}


void ViewManager::OpenLink(string link) {
	if (!link.length()) {
		return;
	}

	// Google News-links (and possibly others) are formatted
	// as such: "http://www.google.com/?.....&url=http://...../&x=y".
	// This brings you to a redirect-notice when opened via 
	// system commands. These lines identify such links and 
	// extracts the ACTUAL url. No GET-parameters can be passed
	// to the contained link, so it's safe to extract any URL 
	// between a "=" and a "&".
	size_t start = link.find("=http://", 4);
	if (start != string::npos) {
		size_t end = link.find("&", ++start);
		if (end == string::npos) {
			end = link.length();
		}
		
		link = link.substr(start, end-start);
	}

	string cmd = "gnome-open ";
	cmd += link;

	FILE *fp;
	fp = popen(cmd.c_str(), "r");
	if (fp) {
		pclose(fp);
	}
}


void ViewManager::DivideString(string str, vector<string> &vec,
								int maxLen) {
								
	if (str == "") return;
	
	int start = 0;
	while (start < str.length()) {
		int remaining = (int)str.length() - start;
		int sublen = min(remaining, maxLen);
		
		/* Only split the strings at spaces */
		if (sublen == maxLen) {
			char *c = &str[start+sublen];
			while ((*c != ' ' && *c != '\n') && sublen > 0) {
				sublen--;
				c--;
			}
			
			/* Error handling - "sublen <= 0" */
			if (sublen <= 0) {
				sublen = maxLen-1;
			}
		}
		
		/* Cut and print the substring */
		string sub = str.substr(start, sublen);
		
		vec.push_back(sub);
		start += sublen+1;
	}
}
