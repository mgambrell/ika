
#pragma once

#include <map>
#include "common/utility.h"
#include "Canvas.h"

struct File;

/**
 * Hardware independant representation of a sprite.
 *
 * Note that the data structure allows for each frame to have its own dimensions.
 * The file format does too, however we won't be actually implementing this yet.
 *
 * This is TERRIBLE.  TODO: unscrew. :P
 */
struct CCHRfile {
    typedef std::map<std::string, std::string> StringMap;
    StringMap               moveScripts;
    StringMap               metaData;    ///< Authoring information and the like.
    
    CCHRfile(int width = 16, int height = 16);
    ~CCHRfile();

    void ClearFrames();

    Canvas& GetFrame(uint frame) const;
    const std::vector<Canvas*>& GetAllFrames();
    void UpdateFrame(const Canvas& newdata, uint nFrame);
    inline int Width(int nFrame = 0)  const   {   return nWidth;  }
    inline int Height(int nFrame = 0) const   {   return nHeight; }
    inline uint NumFrames()           const   {   return frame.size();            }
    
    inline int& HotX(int frame = 0)           {   return nHotx;   }       
    inline int& HotY(int frame = 0)           {   return nHoty;   }     ///< Hotspot position
    inline int& HotW(int frame = 0)           {   return nHotw;   }
    inline int& HotH(int frame = 0)           {   return nHoth;   }     ///< Hotspot size

    void AppendFrame();                                                 ///< Adds a new, empty frame
    void AppendFrame(Canvas& c);
    void AppendFrame(Canvas* c);                                        ///< Append the frame
    void InsertFrame(uint i);                                           ///< Inserts a new, empty frame at the specified position
    void InsertFrame(uint i, Canvas& c);
    void InsertFrame(uint i, Canvas* c);                                ///< Inserts the image as a new frame at the specified position
    void DeleteFrame(uint i);                                           ///< Removes the specified frame

    void Resize(int width, int height);                                 ///< Resize all the frames

    void New(int framex, int framey);                                   ///< Creates a new sprite of the specified dimensions
    void Load(const std::string& fname);                                ///< Loads sprite data from a file
    void Save(const std::string& fname);                                ///< Writes the sprite data to a file
    void SaveOld(const std::string& fname);                             ///< Writes the sprite data to a file, in VERGE's CHR format

    std::string GetStandingScript(Direction dir);
    std::string GetWalkingScript(Direction dir);

private:

    std::vector<Canvas*>   frame;               ///< frame data

    int        nWidth, nHeight;
    int        nHotx, nHoty;                    ///< hotspot position
    int        nHotw, nHoth;                    ///< hotspot width / height

    void LoadCHR(const std::string& filename);
    void Loadv2CHR(File& f);
    void Loadv4CHR(File& f);
    void Loadv5CHR(File& f);
};

