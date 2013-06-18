#include "RssParser.h"
#include <ncurses.h>

using namespace std;


struct Subview {
	int 			x;
	int 			y;
	int 			w;
	int 			h;
};


class ViewManager {
public:
					ViewManager();
					~ViewManager();

	void 			Redraw();

private:
	vector<ItemManager*> itemMgrs;
	vector<RssItem*> 	 displayItems;
	vector<Subview>	 	 subviews;

	int 			dimx;
	int 			dimy;

	void 			CalculateDimensions();
	void 			ReloadItems();
	void 			AssignDisplayItems(int count);
	void 			DrawSubview(int idx);
	void 			DrawEdges();

	void 			ClearManagers();

	void 			DivideString(string str, vector<string> &vec,
								 int maxLen);
};