#include "ViewManager.h"
#include "FileManager.h"
#include "RssParser.h"


#define SUBVIEW_W	40
#define SUBVIEW_H 	7


bool Subview::colPairsInit = false;

Subview::Subview() {
	if (!colPairsInit && has_colors()) {
		for (int i=1; i<=7; i++) {
			init_pair(i, i, 0);
		}

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
}


ViewManager::~ViewManager() {
	ClearManagers();
	endwin();
}


void ViewManager::Redraw() {
	erase();

	CalculateDimensions();
	ReloadItems();
	AssignDisplayItems(subviews.size());

	for (int i=0; i<subviews.size(); i++) {
		DrawSubview(i);
	}

	DrawEdges();

	refresh();
}




void ViewManager::CalculateDimensions() {
	getmaxyx(stdscr, dimy, dimx);
	subviews.clear();

	// Subviews draw their top and left edge.
	// The rightmost and bottommost edges are
	// not allocated to subviews, but drawn by
	// itself.
	int numX = (dimx-1) / SUBVIEW_W;
	int numY = (dimy-1) / SUBVIEW_H;

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


void ViewManager::ReloadItems() {
	vector< pair<string,string> > regs;
	regs = FileManager::GetRegisteredSources();

	ClearManagers();

	for (int i=0; i<regs.size(); i++) {
		string source = regs[i].second;

		ItemManager *mgr = new ItemManager(source);
		
		if (mgr->Reload() >= 0) {
			itemMgrs.push_back(mgr);
		}
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


void ViewManager::DrawSubview(int idx) {
	Subview *sub = &subviews[idx];
	RssItem *item = NULL;

	// Draw the subview
	for (int x=sub->x; x<sub->x+sub->w; x++) {
		mvaddch(sub->y, x, '#');
	}

	for (int y=sub->y; y<sub->y+sub->h; y++) {
		mvaddch(y, sub->x, '#');
	}

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


void ViewManager::ClearManagers() {
	for (int i=0; i<itemMgrs.size(); i++) {
		delete itemMgrs[i];
	}

	itemMgrs.clear();
}


void ViewManager::DivideString(string str, vector<string> &vec,
								int maxLen) {
	int start = 0;
	while (start < str.length()) {
		int remaining = (int)str.length() - start;
		int sublen = min(remaining, maxLen);
		
		/* Only split the strings at spaces */
		if (sublen == maxLen) {
			char *c = &str[start+sublen];
			while ((*c != ' ' && *c != '\n') && sublen >= 0) {
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