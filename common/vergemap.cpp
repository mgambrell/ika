
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

#include "map.h"
#include "misc.h"

static u16 fgetw(FILE* f)
{
    u16 s = 0;
    fread(&s, 1, 2, f);
    return s;
}

static u32 fgetq(FILE* f)
{
    u32 q = 0;
    fread(&q, 1, 4, f);
    return q;
}

Map* ImportVerge1Map(const std::string& fileName)
{
    FILE* f = fopen(fileName.c_str(), "rb");
    if (!f)
        return 0;

    Map* map = new Map;

    try
    {
        u8 magic = fgetc(f);
        if (magic != 4)
            throw std::runtime_error(va(
                "Magic number was %i, not 4.  This is not a v1 map.",
                magic));

        char buffer[256];
        std::fill(buffer, buffer + 256, 0);

        fread(buffer, 1, 13, f);    map->tileSetName = buffer;
        fread(buffer, 1, 13, f);    map->metaData["music"] = buffer;

        int layerCount = fgetc(f);
        int pmulx = fgetc(f);
        int pmuly = pmulx;
        int pdivx = fgetc(f);
        int pdivy = pdivx;

        fread(buffer, 1, 30, f);    map->metaData["name"] = buffer;

        bool showName = fgetc(f) != 0;   map->metaData["showname"] = showName ? "true" : "false";
        bool saveFlag = fgetc(f) != 0;   map->metaData["saveflag"] = saveFlag ? "true" : "false";
        map->metaData["startx"] = ToString(fgetw(f));
        map->metaData["starty"] = ToString(fgetw(f));
        map->metaData["hide"] = ToString(fgetc(f));
        map->metaData["warp"] = ToString(fgetc(f));

        int xsize = fgetw(f);
        int ysize = fgetw(f);

        map->width = xsize * 16;
        map->height = ysize * 16;

        fgetc(f); // toss compression flag

        fseek(f, 27, SEEK_CUR); // "skip buffer" (beats me)
        ScopedArray<u16> lay0 = new u16[xsize * ysize];
        ScopedArray<u16> lay1 = new u16[xsize * ysize];
        ScopedArray<u8>  obs  = new u8[xsize * ysize];
        fread(lay0.get(), sizeof(u16), xsize * ysize, f);
        fread(lay1.get(), sizeof(u16), xsize * ysize, f);
        fread(obs.get(),  sizeof(u8),  xsize * ysize, f);

        // copy 16 bit indeces into a 32 bit buffer
        ScopedArray<uint> layBuffer = new uint[xsize * ysize];
        std::copy(lay0.get(), lay0.get() + xsize * ysize, layBuffer.get());
        Map::Layer* layer0 = new Map::Layer("BG", xsize, ysize);
        layer0->tiles = Matrix<uint>(xsize, ysize, layBuffer.get());

        // FIXME: sometimes we want the obstructions on layer 1, not 0
        layer0->obstructions = Matrix<u8>(xsize, ysize, obs.get());
        
        std::copy(lay1.get(), lay1.get() + xsize * ysize, layBuffer.get());
        Map::Layer* layer1 = new Map::Layer("FG", xsize, ysize);
        layer1->tiles = Matrix<uint>(xsize, ysize, layBuffer.get());

        map->AddLayer(layer0);
        map->AddLayer(layer1);

        struct VergeZone
        {
            char name[15];
            u16  eventIndex;
            u8   chance;
            u8   delay;
            u8   adjacentActivate;
            char description[30];
        };

        // TODO: do something useful with the zone stuff
        fseek(f, sizeof(VergeZone) * 128, SEEK_CUR);

        std::fill(buffer, buffer + 256, 0);
        std::vector<std::string> chrlist;
        for (int i = 0; i < 100; i++)
        {
            fread(buffer, 1, 13, f);
            chrlist.push_back(std::string(buffer));
        }

        int numEntities = 0;
        fread(&numEntities, 1, 4, f);

        struct VergeEntity
        {
            u16 x;       
            u16 y;       
            u8 direction;
            u8 moveDir;  
            u8 moveCount;
            u8 frameCount;
            u8 specFrame;
            u8 spriteIndex;
            u8 moveCode;
            u8 adjacentActivate;
            u8 ghost;
            uint actScriptIndex;
            uint moveScriptIndex;
            u8 speed;
            u8 speedCount;

            u16 data[8]; // hell if I know
            u16 boundingBox[4];
            u8 curCommand;
            u8 commandArg;
            u8* scriptofs;
            u8 facePlayer;
            u8 chasePlayer;
            u8 chaseSpeed;
            u8 chaseDist;
            u16 tileX;
            u16 tileY;
            int omfg;
            char description[20];                 // Editing description
        };

        for (int i = 0; i < numEntities; i++)
        {
            VergeEntity vergeEnt;
            Map::Entity ikaEnt;

            fread(&vergeEnt, 1, 88, f); // don't ask where the 88 comes from :P
            ikaEnt.label = va("Ent%i", i);
            ikaEnt.x = vergeEnt.x * 16;
            ikaEnt.y = vergeEnt.y * 16;
            ikaEnt.direction = (Direction)vergeEnt.direction;
            ikaEnt.spriteName = chrlist[vergeEnt.spriteIndex];
            ikaEnt.obstructedByEntities =
                ikaEnt.obstructedByMap =
                ikaEnt.obstructsEntities = !vergeEnt.ghost;
            ikaEnt.activateScript = va("Event%i", vergeEnt.actScriptIndex);
            layer0->entities.push_back(ikaEnt);
        }

        u8 numMoveScripts = fgetc(f);
        int thingie = fgetq(f);
        fseek(f, numMoveScripts, SEEK_CUR); // skip movescripts
        fseek(f, thingie, SEEK_CUR);    // and other shit (who knows what the hell it is)

        fclose(f);
        return map;
    }
    catch (...)
    {
        fclose(f);
        throw;
    }
}
