#include "RssParser.h"
#include <ncurses.h>
#include <map>

using namespace std;


struct Subview {
					Subview();

					// Calculate a color value based
					// on the text to give the same item
					// the same color every time.
	int 			GetColorPair(string title, string desc);

	int 			x;
	int 			y;
	int 			w;
	int 			h;

private:
	static bool 	colPairsInit;	
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