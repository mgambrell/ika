/*
    OpenGL stuff isn't as fast as I had hoped it'd be.

    Since all the OpenGL stuff is windowed, we can take advantage of the fact
    that all page flipping is done via block copying, and thus we can safely
    assume that the framebuffer is not erased when "flipping", by implementing
    dirty rectangle based rendering.
    
    Winmaped used this technique to fantastic effect; it was still immediately
    responsive when maximized at 1600x1200x32 when setting tiles on a map, and
    that was done with software rendering.
*/

#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "types.h"
#include <map>
#include <wx\wx.h>
#include "docview.h"

class Map;

class CMainWnd;
class CGraphFrame;
class CTileSet;
class CLayerVisibilityControl;

class wxSashLayoutWindow;
class wxCheckListBox;
class wxScrolledWindow;

class CMapView : public IDocView
{
    enum
    {
        lay_entity=-10,
        lay_zone,
        lay_obstruction
    };

    enum
    {
        hidden=0,
        visible,
        darkened,
    };

    enum CursorMode
    {
        mode_normal,
        // copy/paste/etc...
    };

    CMainWnd*           pParentwnd;

    wxSashLayoutWindow* pLeftbar;
    wxSashLayoutWindow* pRightbar;
    wxScrolledWindow*   pScrollwin;
    CGraphFrame*        pGraph;
    CLayerVisibilityControl*
                        pLayerlist;

    Map*                pMap;
    CTileSet*           pTileset;

    int  nZoom;                                                 // in 16ths (ie 16 is 1:1, while a value of 1 means 1:16)

public:
    CMapView(CMainWnd* parent,const string& fname);

    void InitLayerVisibilityControl();

    void OnPaint();
    void OnErase(wxEraseEvent&) {}
    void OnSize(wxSizeEvent& event);
    void OnScroll(wxScrollWinEvent& event);

    void OnSave(wxCommandEvent& event) {}

    void OnClose();

//------------------------------------------------------------

//------------------------------------------------------------

    DECLARE_EVENT_TABLE()

public:
    // CLayerVisibilityControl calls these functions
    void OnLayerChange(int lay);
    void OnLayerToggleVisibility(int lay,int newstate);

    // Stuff that's not directly related to the UI

private:
    int xwin,ywin;
    int nCurlayer;
    // old mouse coords.  To prevent redundant processing.
    int oldx,oldy;
    CursorMode          csrmode;
    std::map<int,int> nLayertoggle;

    void ScreenToMap(int& x,int& y);
    void HandleLayerEdit(wxMouseEvent& event);
    void HandleMouse(wxMouseEvent& event);
    
    void Render();
    void RenderLayer(int lay);

    bool Save(const char* fname);
};

#endif