
#ifndef TILESETSTATE_H
#define TILESETSTATE_H

#include "common/utility.h"
#include "editstate.h"

struct CompositeCommand;

struct TileSetState : EditState
{
public:
    TileSetState(Executor* e);

    virtual void OnMouseDown(wxMouseEvent& event);
    virtual void OnMouseUp(wxMouseEvent& event);
    virtual void OnMouseMove(wxMouseEvent& event);
    virtual void OnMouseWheel(wxMouseEvent& event);
    virtual void OnRenderCurrentLayer();

    // lowest common denominator, since more than one of the above methods will be setting tiles.
    // x and y are still in client coordinates.  _curTile is the tile that is set.
    void SetTile(int x, int y);

    uint GetCurTile() const;
    void SetCurTile(uint t);

private:
    int _oldX, _oldY;

    CompositeCommand* _curGroup;
};

#endif