#include "editstate.h"
#include "executor.h"
#include "mapview.h"
#include "tilesetview.h"
#include "common/map.h"
#include "tileset.h"
#include "command.h"

EditState::EditState(Executor* e, std::string name)
    : _executor(e)
    , _name(name)
{
}

Executor*    EditState::GetExecutor()       const { return _executor; }
MapView*     EditState::GetMapView()        const { return _executor->GetMapView(); }
TilesetView* EditState::GetTilesetView()    const { return _executor->GetTilesetView(); }
Map*         EditState::GetMap()            const { return _executor->GetMap(); }

Map::Layer*  EditState::GetCurLayer() const
{
    if (_executor->GetMap()->NumLayers() == 0)
        return 0;
    else
        return _executor->GetMap()->GetLayer(_executor->GetCurrentLayer());
}


std::string EditState::GetName()
{
    return _name;
}


uint      EditState::GetCurLayerIndex()     const { return _executor->GetCurrentLayer(); }
Tileset*  EditState::GetTileset()           const { return _executor->GetTileset(); }
wxWindow* EditState::GetParentWindow()      const { return _executor->GetParentWindow(); }

void EditState::HandleCommand(::Command* cmd)
{
    _executor->HandleCommand(cmd);
}
