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

	void 			MainLoop();
	void 			Redraw();
	void 			ReloadItems();

private:
	vector<ItemManager*> itemMgrs;
	vector<RssItem*> 	 displayItems;
	vector<Subview>	 	 subviews;

	int 			selectedIndex;
	int 			dimx, dimy;
	int 			numX, numY;

	void 			HandleInput();

	void 			CalculateDimensions();
	void 			AssignDisplayItems(int count);
	void 			DrawSubview(int idx, int fillColorPair = 7);
	void 			DrawSelection();
	void 			DrawEdges();
	void 			FillSubview(int subview, int colorPair);

	void 			ClearManagers();
	void 			OpenLink(string link);

	void 			DivideString(string str, vector<string> &vec,
								 int maxLen);
};