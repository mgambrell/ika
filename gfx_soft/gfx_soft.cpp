#include "gfx_soft.h"
#include "blits.h"
#include "log.h"
#include "types.h"

// DirectDraw for fullscreen rendering
LPDIRECTDRAW		lpdd=NULL;						// Main DirectDraw object
LPDIRECTDRAWSURFACE	mainsurface=NULL;				// front buffer
LPDIRECTDRAWSURFACE backsurf=NULL;					// back buffer for flipping
LPDIRECTDRAWCLIPPER	mainclip=NULL;					// clippin
LPDIRECTDRAWGAMMACONTROL	lpgamma=NULL;			// gamma controller, for palettemorph
DDGAMMARAMP			normalgamma;					// typical gamma settings
DDGAMMARAMP			currentgamma;					// active gamma settings

// DIB for windowed rendering

HBITMAP hBitmap;
void*	pBackbuffer;
HDC     hDC;

int zoomfactor=1;									// zoom multiplier

HWND hCurWnd;

int nBytesperpixel;
int xres,yres;
bool bFullscreen;
handle hScreen;										// Handle to the screen image
handle hRenderdest;									// handle to the image that we blit to
RECT maincliprect;

int nScreensurfacewidth,nScreensurfaceheight;		// width/height of the screen surface. (we have to remember this)

static int nImagecount=0;							// Running count of images, for debugging purposes
static bool bInited=false;

inline RECT Rect(int x1,int y1,int x2,int y2)
{
    RECT r;
    r.left=x1; r.top=y1;
    r.right=x2; r.bottom=y2;
    return r;
}

inline void ValidateClipRect(handle& img)
{
    if (img->rClip.left<0) img->rClip.left=0;
    if (img->rClip.top<0)  img->rClip.top=0;
    if (img->rClip.right>img->nWidth) img->rClip.right=img->nWidth;
    if (img->rClip.bottom>img->nHeight) img->rClip.bottom=img->nHeight;
}

inline int Blend_Pixels(u32 c1,u32 c2)
{
    u32 c;
    
    u32 a=c1>>24;
    
    c =( a*((c1&255)-(c2&255))/256 + (c2&255) );
    c1>>=8; c2>>=8;
    c|=( a*((c1&255)-(c2&255))/256 + (c2&255) )<<8;
    c1>>=8; c2>>=8;
    c|=( a*((c1&255)-(c2&255))/256 + (c2&255) )<<16;
    
    c|=255<<24;							// max out alpha
    
    return c;
}

/////////////////// Code ////////////////////////////////

int gfxGetVersion()	{	return 2;	}

bool gfxInit(HWND hWnd,int x,int y,int,bool fullscreen)
{
    initlog("gfx_soft.log");
    
    if (bInited)
    {
        log("redundant gfxInit's");
        return false;
    }
    
    xres=x; yres=y;
    nBytesperpixel=4;
    hCurWnd=hWnd;
    bFullscreen=fullscreen;
    
    if (fullscreen)
    {
        HRESULT result;
        DDSURFACEDESC ddsd; 
        
        if (!lpdd)
        {
            result=DirectDrawCreate(NULL,&lpdd,NULL);
            if (result!=DD_OK) return false; // poo
        }
        try
        {
            result=lpdd->SetCooperativeLevel(hWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
            if (FAILED(result)) throw result;
            
            result=lpdd->SetDisplayMode(x,y,32);
            if (FAILED(result)) throw result;
            
            ZeroMemory(&ddsd,sizeof ddsd);
            ddsd.dwSize=sizeof ddsd;
            ddsd.dwFlags=DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
            ddsd.ddsCaps.dwCaps=DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
            ddsd.dwBackBufferCount=1;
            result=lpdd->CreateSurface(&ddsd,&mainsurface,NULL);
            if (FAILED(result)) throw result;
            
            DDSCAPS ddscaps;
            ZeroMemory(&ddscaps,sizeof ddscaps);
            ddscaps.dwCaps=DDSCAPS_BACKBUFFER;
            result=mainsurface->GetAttachedSurface(&ddscaps,&backsurf);
            if (FAILED(result)) throw result;
            
            log("Initing gamma stuff...");
            result=mainsurface->QueryInterface(IID_IDirectDrawGammaControl,(void**)&lpgamma);
            if (result!=S_OK)
            {
                lpgamma=NULL;
                log("gfxInit: gamma init stuff failed!! :o");
            }
            else
            {
                log("Gamma stuff inited okay");
                lpgamma->GetGammaRamp(0,&normalgamma);
                lpgamma->GetGammaRamp(0,&currentgamma);
            }
        }
        catch(HRESULT hr)
        {
            logdderr(hr);
            gfxShutdown();
            return false;
        }
    }
    else
    {
        try
        {
            hDC=CreateCompatibleDC(NULL);
            if (!hDC)
                throw "Couldn't create DC";
            
            BITMAPINFO bmi;
            BITMAPINFOHEADER& bmih = bmi.bmiHeader;
            ZeroMemory(&bmi,sizeof bmi);
            bmih.biSize=sizeof(bmih);
            bmih.biWidth=xres;
            bmih.biHeight=-yres;
            bmih.biPlanes=1;
            bmih.biBitCount=32;
            bmih.biCompression=BI_RGB;
            
            hBitmap=CreateDIBSection(hDC,&bmi,DIB_RGB_COLORS,&pBackbuffer,NULL,0);
            if (!hBitmap)
                throw "Couldn't create DIB section";
            
            SelectObject(hDC,hBitmap);
            
            MakeClientFit();
        }
        catch(const char* msg)
        {
            log("gfxInit error: %s",msg);
            gfxShutdown();
            return false;
        }
    }
    
    hScreen=gfxCreateImage(xres,yres);
    
    hRenderdest=hScreen;
    
    gfxClipWnd(0,0,x-1,y-1);
    
    log("gfxinit finished");
    
    bInited=true;
    return true;
}

void gfxShutdown()
{
    if (!bInited)
        return;
    
    bInited=false;
    
    gfxFreeImage(hScreen);
    
    if (bFullscreen)
    {
        lpdd->RestoreDisplayMode();
        if (mainsurface) mainsurface->Release();
        if (mainclip)    mainclip->Release();
        if (lpgamma)	 {	lpgamma->SetGammaRamp(0,&normalgamma);	lpgamma->Release();	}
        if (lpdd)        lpdd->Release();
        
        mainsurface=NULL;
        mainclip=NULL;
        lpgamma=NULL;
        lpdd=NULL;
    }
    else
    {
        DeleteDC(hDC);
        DeleteObject(hBitmap);
    }
    
    if (nImagecount!=0)
        log("gfxShutdown:: %i image%s unaccounted for!!!",nImagecount,nImagecount==1?"s":"");
}

bool gfxSwitchToFullScreen()
{
    return 0;
}

bool gfxSwitchToWindowed()
{
    return 0;
}

bool gfxSwitchResolution(int x,int y)
{
    //	if (hScreen)
    //		return false;
    
    log("-------resize------------");
    if (bFullscreen)
    {
        // NYI
        
    }
    else
    {
        xres=x;
        yres=y;
        
        DeleteObject(hBitmap);
        
        BITMAPINFO bmi;
        BITMAPINFOHEADER& bmih = bmi.bmiHeader;
        ZeroMemory(&bmi,sizeof bmi);
        bmih.biSize=sizeof(bmih);
        bmih.biWidth=xres;
        bmih.biHeight=-yres;
        bmih.biPlanes=1;
        bmih.biBitCount=32;
        bmih.biCompression=BI_RGB;
        
        hBitmap=CreateDIBSection(GetDC(NULL),&bmi,DIB_RGB_COLORS,&pBackbuffer,NULL,0);
        
        SelectObject(hDC,hBitmap);
    }
    return false;
}

handle gfxCreateImage(int x,int y)
{
    handle temp;
    
    temp=new SImage;
    
    temp->pData=new u32[x*y];
    ZeroMemory(temp->pData,x*y*sizeof(u32));
    temp->nWidth=x;
    temp->nHeight=y;
    temp->nPitch=x;
    temp->Blit=&NullBlit;
    temp->ScaleBlit=&NullScaleBlit;
    
    gfxClipImage(temp, Rect(0,0,x,y) );
    nImagecount++;
    
    return temp;
}

bool gfxFreeImage(handle img)
{
    if (!img)
        return false;
    
    delete[] img->pData;
    img->pData=NULL;
    img->nWidth=img->nHeight=img->nPitch=0;
    nImagecount--;
    
    delete img;
    
    return true;
}

handle gfxCopyImage(handle img)
{
    bool gfxCopyPixelData(handle,u32*,int,int);
    
    handle i=gfxCreateImage(img->nWidth,img->nHeight);
    gfxCopyPixelData(i,img->pData,img->nWidth,img->nHeight);
    
    nImagecount++;
    
    return i;
}

bool gfxCopyPixelData(handle img,u32* data,int width,int height)
{
    
    delete[] img->pData;
    
    img->pData=new u32[width*height];
    
    img->nWidth=width;
    img->nHeight=height;
    img->nPitch=width;
    img->rClip=Rect(0,0,width,height);
    
    for (int y=0; y<height; y++)
        for (int x=0; x<width; x++)
        {
            u32 c=*data++;
            
            int r,g,b,a;
            a=c>>24;
            r=(c>>16)&255;
            g=(c>>8)&255;
            b=c&255;
            
            img->pData[y*width+x]=(a<<24)|(b<<16)|(g<<8)|r;
        }
        
        ScanImage(img);
        
        return 0;
}

bool gfxClipWnd(int x1,int y1,int x2,int y2)
{
    maincliprect.left=x1;
    maincliprect.top=y1;
    maincliprect.right=x2;
    maincliprect.bottom=y2;
    return true;
}

bool gfxClipImage(handle img,RECT r)
{
    img->rClip=r;
    ValidateClipRect(img);
    return 0;
}

handle gfxGetScreenImage()
{
    return hScreen;
}

bool gfxShowPage()
{
    if (bFullscreen)
    {
        HRESULT result;
        DDSURFACEDESC ddsd;
        
        // First, copy the screen to the back buffer
        ZeroMemory(&ddsd,sizeof ddsd);
        ddsd.dwSize=sizeof ddsd;
        result=backsurf->Lock(NULL,&ddsd,DDLOCK_WAIT | DDLOCK_WRITEONLY,NULL);
        if (FAILED(result))	{ log("lockfail"); return false; }
        
        u32* pSrc =hScreen->pData;
        u8* pDest=(u8*)ddsd.lpSurface;
        //	int xlen=xres>ddsd.dwWidth?xres:ddsd.dwWidth;
        int xlen=xres;
        xlen*=sizeof(u32);
        for (int y=yres; y; y--)
        {
            memcpy(pDest,pSrc,xlen);
            pSrc+=hScreen->nWidth;
            pDest+=ddsd.lPitch;
        }
        
        result=backsurf->Unlock(ddsd.lpSurface);
        if (FAILED(result)) return false;
        
        result=mainsurface->Flip(NULL,DDFLIP_WAIT);
        if (FAILED(result))
        {
            log("flipfail");
            return false;
        }
    }
    else
    {
        int xlen=xres*sizeof(u32);
        int ylen=yres;
        
        u8* pDest=(u8*)pBackbuffer;
        u8* pSrc=(u8*)hScreen->pData;
        
        while (ylen--)
        {
            memcpy(pDest,pSrc,xlen);
            pDest+=xlen;			pSrc+=xlen;
        }
        
        HDC hCurWndDC=GetDC(hCurWnd);
        BitBlt(hCurWndDC,0,0,xres,yres,hDC,0,0,SRCCOPY);
        ReleaseDC(hCurWnd,hCurWndDC);
        
        return true;
    }
    return false; // should never execute, but MSVC is a bitch
}

bool gfxBlitImage(handle img,int x,int y,bool transparent)
{
#ifdef _DEBUG
    if (!img->Blit)
    {
        log("as;dlkjfas;dlkjf");
        return false;
    }
#endif
    
    if (!transparent)
        return OpaqueBlit(img,x,y);
    
    return img->Blit(img,x,y);
}

bool gfxScaleBlitImage(handle img,int cx,int cy,int w,int h,bool transparent)
{
#ifdef _DEBUG
    if (!img->ScaleBlit)
    {
        log("dasdasdf");
        return false;
    }
#endif
    
    if (!transparent)
        return OpaqueScaleBlit(img,cx,cy,w,h);
    
    return img->ScaleBlit(img,cx,cy,w,h);
}

bool gfxRotScaleImage(handle,int,int,float,int,bool)
{
    return false;
}

bool gfxCopyChan(handle src,int nSrcchan,handle dest,int nDestchan)
{
#ifdef _DEBUG
    if (!src || !dest)
    {
        log("CopyChan: eeeeeeeeeeeeeeeeeeeeek!");
        return false;
    }
#endif
    
    int xlen=src->nWidth  < dest->nWidth   ?  src->nWidth  : dest->nWidth;									// whichever is smaller
    int ylen=src->nHeight < dest->nHeight  ?  src->nHeight : dest->nHeight;
    int srcinc=(src->nPitch-src->nWidth)*4;
    int destinc=(dest->nPitch-dest->nWidth)*4;
    
    u8* pSrc=(u8*)src->pData+nSrcchan;
    u8* pDest=(u8*)dest->pData+nDestchan;
    
    while (ylen--)
    {
        int x=xlen;
        while (x--)
        {
            *pDest=*pSrc;
            pDest+=4;
            pSrc+=4;
        }
        pSrc+=srcinc;
        pDest+=destinc;
    }
    
    return true;
}

extern void RescanImage(handle img,bool bZero,bool bFull,bool bPartial);

bool gfxSetRenderDest(handle img)		// careful, the driver can't validate this
{
    hRenderdest=img;
    return true;
}

handle gfxGetRenderDest()
{
    return hRenderdest;
}

int  gfxImageWidth(handle img)
{
    return img->nWidth;
}

int  gfxImageHeight(handle img)
{
    return img->nHeight;
}

bool gfxPaletteMorph(int r,int g,int b)
{
    if (bFullscreen)
    {
        int count;
        
        if (lpgamma)
        {
            for (count=0; count<256; count++)
            {
                currentgamma.red[count]=(u16)(count*r);
                currentgamma.green[count]=(u16)(count*g);
                currentgamma.blue[count]=(u16)(count*b);
            }
            
            lpgamma->SetGammaRamp(0,&currentgamma);
            return true;
        }
        else
            return false;
    }
    return false;
}

////////////////////// Implementation helper function type things /////////////////////////

void keepinrange(int& i,int min,int max)
{
    if (i<min) i=min;
    if (i>max) i=max;
}

#include "primatives.h"

void MakeClientFit()
{
    RECT client,window,goal;
    int ox,oy; // how far off are we?
    
    if (bFullscreen) return;  // why?
    goal.left=goal.top=0;
    goal.right=xres; goal.bottom=yres;
    
    GetClientRect(hCurWnd,&client);
    GetWindowRect(hCurWnd,&window);
    
    // find out how much adjustment we need to do
    ox=xres-client.right;
    oy=yres-client.bottom;
    
    // do it!
    window.right+=ox;
    window.bottom+=oy;
    
    POINT pt;
    
    pt.x=pt.y=0;
    
    ClientToScreen(hCurWnd,&pt);
    
    MoveWindow(hCurWnd,window.left,window.top,window.right-window.left,window.bottom-window.top,true);
}

/*LPDIRECTDRAWSURFACE CreateOffScreenSurface(int x,int y)
// Creates an off-screen surface, and returns a pointer to it. ;)
// width and height are the dimensions.  The pixel format is the same as the primary surface's.
{
LPDIRECTDRAWSURFACE temp;
DDSURFACEDESC ddsd;
HRESULT result;
temp=NULL;

  ddsd.dwSize=sizeof ddsd;
  ddsd.dwFlags=DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
  ddsd.dwWidth=x;
  ddsd.dwHeight=y;
  ddsd.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
  result=lpdd->CreateSurface(&ddsd,&temp,NULL);
  
    if (result==DD_OK)
    return temp;
    else
    return NULL;
}*/
