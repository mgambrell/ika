/*
	This is a class that will allow a single window to... have graphics on it. :P

	It renders the window in arbitrary rectangular regions, and.. uh... stuff.

	32bpp ONLY, but since DIBs are device independant, it should display properly on all displays.
*/

#ifndef GRAPH_H
#define GRAPH_H

#include <windows.h>
#include <vector>
#include "pixel_matrix.h"
#include "dib.h"

class CGraphView
{
private:
	// ----------types----------
	struct point										// A lil' struct to store what we need to store for a single block
	{
		int x,y;
		point(int initx,int inity) { x=initx; y=inity; }
	};
	typedef void(*renderfunc)(void* pThis,const RECT& r);

	// ---------Data----------
	HWND hWnd;
	CDIB* pDib;
	renderfunc Render;									// called when we want to render something

	void* pThis;
		
	std::vector<point> dirtyrects;						// And a list of blocks that have been altered
	void AddBlock(int x,int y);							// just in case we want to get fancy and avoid redundantly setting a block later on
	void AlphaBlit(const CPixelMatrix& src,int x,int y);// just so that we don't have both opaque and transparent crap being blitted in one long, ugly function :)
	void CGraphView::DoClipping(int& x,int& y,int& xstart,int& xlen,int& ystart,int& ylen);

public:
	CGraphView(HWND hwnd,renderfunc pRenderfunc,void* pthis);
	~CGraphView();

	// drawing functions
	void Blit(const CPixelMatrix& src,int x,int y,bool trans);
	// void scaleblit?
	void HLine(int x1,int x2,int y,u32 colour);
	void VLine(int x,int y1,int y2,u32 colour);
	void Rect(int x1,int y1,int x2,int y2,u32 colour);
	void Stipple(int x1,int y1,int x2,int y2,u32 colour);

	void Clear();

	// Rendering stuff
	void DirtyRect(RECT r);								// dirties everything in the specified rect, so that the next ShowPage will redraw that section
	void ShowPage();									// renders dirty rects
	void ShowPage(const RECT& r);						// renders the specified rect
	void ForceShowPage();								// rerenders the whole window
};

#endif