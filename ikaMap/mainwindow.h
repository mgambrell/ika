
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <wx/wx.h>
#include <stack>
#include <map>

#include "spriteset.h"
#include "controller.h"

class wxSashLayoutWindow;
class wxSashEvent;
class wxCheckListBox;
class Command;
class MapView;
class TileSetView;
struct Map;
class TileSet;

/**
 * The main application frame.
 * The ultimate source of all the actual map resources. (sprites, the tileset, and the map itself)
 * Also deals with the layer list, tool buttons, drop down menu, and the Command interface used
 * to actually modify the map.
 */
class MainWindow : public wxFrame
{
    friend class ChangeTileSetCommand;

private:
    wxSashLayoutWindow* _topBar;
    wxSashLayoutWindow* _bottomBar;
    wxSashLayoutWindow* _sideBar;
    wxCheckListBox*     _layerList;
    MapView* _mapView;
    TileSetView* _tileSetView;
    
    std::map<std::string, SpriteSet*> _sprites;
    
    std::stack<::Command*> _undoList;
    std::stack<::Command*> _redoList;

    static const float _version;
    std::string _curMapName;

    // helper function for clearing the undo or redo list.  Deletes Commands as it does so, to avoid leaks.
    static void ClearList(std::stack<::Command*>& list);

protected:
    Map* _map;
    TileSet* _tileSet;

public:
    MainWindow(
        const wxPoint& position = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        const long style = wxDEFAULT_FRAME_STYLE);
    ~MainWindow();

    void OnClose(wxCloseEvent&);
    void OnSize(wxSizeEvent&);
    void OnDragSash(wxSashEvent& event);

    void OnNewMap(wxCommandEvent&);
    void OnOpenMap(wxCommandEvent&);
    void OnSaveMap(wxCommandEvent&);
    void OnSaveMapAs(wxCommandEvent&);
    void OnLoadTileSet(wxCommandEvent&);
    void OnSaveTileSetAs(wxCommandEvent&);
    void OnExportTileSet(wxCommandEvent&);
    void OnExit(wxCommandEvent&);
    void OnUndo(wxCommandEvent&);
    void OnRedo(wxCommandEvent&);
    void OnEditMapProperties(wxCommandEvent&);
    void OnImportTiles(wxCommandEvent&);
    void OnChangeCurrentLayer(wxCommandEvent& event);
    void OnShowLayerProperties(wxCommandEvent& event);
    void OnZoomMapIn(wxCommandEvent&);
    void OnZoomMapOut(wxCommandEvent&);
    void OnZoomMapNormal(wxCommandEvent&);
    void OnZoomTileSetIn(wxCommandEvent&);
    void OnZoomTileSetOut(wxCommandEvent&);
    void OnZoomTileSetNormal(wxCommandEvent&);

    void OnToggleLayer(wxCommandEvent& event);
    void OnSetTilePaintState(wxCommandEvent&);
    void OnSetObstructionState(wxCommandEvent&);
    void OnSetEntityState(wxCommandEvent&);

    void OnNewLayer(wxCommandEvent&);
    void OnDestroyLayer(wxCommandEvent&);
    void OnMoveLayerUp(wxCommandEvent&);
    void OnMoveLayerDown(wxCommandEvent&);

    void UpdateLayerList();
    void UpdateTitle();

    bool IsLayerVisible(uint index) const;
    void ShowLayer(uint index, bool show = true);
    void HideLayer(uint index) { ShowLayer(index, false); }

    Map* GetMap() const;
    TileSet* GetTileSet() const;
    MapView* GetMapView() const;
    TileSetView* GetTileSetView() const;

    void HandleCommand(::Command* cmd);
    void Undo();
    void Redo();

    void LoadMap(const std::string& fileName);

    SpriteSet* GetSprite(const std::string& fileName);

    DECLARE_EVENT_TABLE()
};

#endif