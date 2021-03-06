#pragma once

//#define SHARE_TEXTURES

#ifdef SHARE_TEXTURES
#   include <set>
#endif

//#define PREMULTIPLY_ALPHA

#include "SDL/include/SDL.h"

#include "../video/Driver.h"
#include "../../common/Canvas.h"
#include "../../common/types.h"

#include "../FPSCounter.h"

#include "Image.h"

/// OpenGL video driver implementation
namespace OpenGL {

    // Don't create stack instances, dork.
    // And keep the refcount up to date
    struct Texture {
        friend struct Driver;

        uint handle;
        int width;
        int height;
        uint refCount;

        Point unused;   // where the next image should go

    protected:
        Texture(uint p = 0, int w = 0, int h = 0)
            : handle(p)
            , width(w)
            , height(h)
            , refCount(0)
            , unused(0, 0)
        {}

    private:
        // NO
        Texture(const Texture&);
        Texture& operator =(const Texture&);
    };

    struct NotYetImplementedException{};
#   define NYI { throw NotYetImplementedException(); }

    /// The driver itself.
    struct Driver : public Video::Driver {

        Driver(int xres, int yres, int bpp, bool fullScreen, bool doubleSize, bool filter);
        ~Driver();

        /// Switches the driver to display fullscreen.
        virtual void SwitchToFullScreen() NYI

        /// Switches the driver to display in a window.
        virtual void SwitchToWindowed() NYI

        /// Changes the resolution of the output.  Returns true on success.
        virtual bool SwitchResolution(int x, int y);

        /// Creates a new image from the provided pixel buffer.
        virtual Image* CreateImage(Canvas &pm);

        /// Frees the previously created image.
        virtual void FreeImage(Video::Image* img);

        /// Clips the image to the provided rectangle.
        virtual void ClipScreen(int left, int top, int right, int bottom);

        /// Returns the current clipping rectangle.
        virtual Rect* GetClipRect();

        /// Flips the buffers, displays the screen, whatever.
        virtual void ShowPage();

        /// Clears the screen!  With blackness!
        virtual void ClearScreen();

        /// Sets the current blend mode.
        virtual Video::BlendMode SetBlendMode(Video::BlendMode bm);

        /// Blits an image to the screen.
        virtual void BlitImage(Video::Image* img, int x, int y);
        
        /// Blits a rectangular piece of an image to the screen.
        virtual void ClipBlitImage(Video::Image* i, int x, int y, int ix, int iy, int iw, int ih);

        /// Blits a scaled version of the provided image to the screen.
        virtual void ScaleBlitImage(Video::Image* img, int x, int y, int w, int h);

        /// Blits a rotated (and possibly scaled) version of the provided image to the screen.
		virtual void RotateBlitImage(Video::Image* img, int x, int y, float angle, float scalex, float scaley);

        /// Blits a distorted version of the image to the screen, given the provided corner points.
        virtual void DistortBlitImage(Video::Image* img, int x[4], int y[4]);

        /// "tile" blits an image to fill the rect specified.
        virtual void TileBlitImage(Video::Image* img, int x, int y, int w, int h, float scalex, float scaley);

        /// Blits the image, using tint as a colour mask thingie.
        virtual void TintBlitImage(Video::Image* img, int x, int y, u32 tint);

        /// DistortBlits an image, using the colour array to tint each corner of the image.  Colours are interpolated
        /// like OpenGL usually does when rendering textured, distorted quads.
        virtual void TintDistortBlitImage(Video::Image* img, int x[4], int y[4], u32 colour[4]);

        /// Combines TintBlit and TileBlit.  'nuff said.
        virtual void TintTileBlitImage(Video::Image* img, int x, int y, int w, int h, float scalex, float scaley, u32 tint);

        /// Draws a single pixel on the screen.
        virtual void DrawPixel(int x, int y, u32 colour);

        /// Gets the value of a pixel on the screen.
        virtual u32 GetPixel(int /*x*/, int /*y*/) NYI

        /// Draws a line on the screen.
        virtual void DrawLine(int x1, int y1, int x2, int y2, u32 colour);

        /// Draws a rectangle on the screen.
        virtual void DrawRect(int x1, int y1, int x2, int y2, u32 colour, bool filled);

        /// Draws an ellipse on the screen.
        virtual void DrawEllipse(int cx, int cy, int rx, int ry, u32 colour, bool filled);

        /// Draws an arc on the screen.
        virtual void DrawArc(int cx, int cy, int rx, int ry, int irx, int iry, int start, int end, u32 colour, bool filled);
        
        /// Draws a triangle on the screen.
        virtual void DrawTriangle(int x[3], int y[3], u32 colour[3]);

        /// Draws a quad on the screen.
        virtual void DrawQuad(int x[4], int y[4], u32 colour[4]);
                
        /// Draws a series of lines on the screen.
        virtual void DrawLineList(std::vector<int> x, std::vector<int> y, std::vector<u32> colour, int drawmode);
                
        /// Draws a triangle on the screen.
        virtual void DrawTriangleList(std::vector<int> x, std::vector<int> y, std::vector<u32> colour, int drawmode);
        
        /// Grabs a rect from the screen, constructs an image from it, and returns it
        virtual Image* GrabImage(int x1, int y1, int x2, int y2);

        /// Like GrabImage, but stores the contents on a canvas, not an image
        virtual Canvas* GrabCanvas(int x1, int y1, int x2, int y2);

        /// Sets the current tint colour.  This tint is applied to *everything*
        virtual u32 GetTint();

        /// Sets the current tint colour. :P
        virtual void SetTint(u32 tint);

        /// Returns the size of the viewport, in pixels.
        virtual Point GetResolution() const;

        /// Returns the number of times ShowPage() has been called in the past second.
        virtual int GetFrameRate() const;

    private:
        FPSCounter fps;
        SDL_Surface* _screen;
        int  _xres;
        int  _yres;
        int  _bpp;
        bool _fullScreen;

        RGBA _tintColour;

        Video::BlendMode _blendMode;

        // If true, we make the real resolution twice normal, and let
        // OpenGL scale it when we ShowPage
        // _bufferTex is used to do the scaling.
        bool _doubleSize;
        uint _bufferTex;
        // If true, use bilinear filtering for doublesize. If false, use nearest neighbor.
        bool _filter;

        uint _lasttex;
        void SwitchTexture(uint tex);

#ifdef SHARE_TEXTURES
        typedef std::set<Texture*> TextureSet;
        TextureSet _textures;  // textures allocated.  Only used for 16x16 images at this moment.
#endif

        void (IKA_STDCALL *glBlendEquationEXT)(int);
    };
};
