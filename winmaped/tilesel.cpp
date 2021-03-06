/* 
Tile selector

  This is a lot better than my old implementation.  No globals, etc...
  
    TODO: add flip/rotate functions
*/

#include "tilesel.h"
#include "log.h"
#include "resource.h"
#include "importvspdlg.h"
#include "miscdlg.h"

const int nGayarbitraryconstant=0;//32;

CTileSel::CTileSel()
{
    nYoffset=0;
    hWnd=0;
    bPad=false;
    active=false;
    
    pGraph=0;
}

CTileSel::~CTileSel()
{
    delete pGraph;
}

BOOL CALLBACK CTileSel::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CTileSel* t;
    t=(CTileSel*)GetWindowLong(hWnd,GWL_USERDATA);
    if (t)
        return t->MainProc(hWnd,msg,wParam,lParam);
    
    return DefWindowProc(hWnd,msg,wParam,lParam);
}

int CTileSel::TileUnderCursor(int x,int y)
{
    RECT temprect;
    int width,height; // window dimensions
    int tx,ty;        // tile dimensions + 1 pixel padding
    
    GetClientRect(hWnd,&temprect);
    width=temprect.right; height=temprect.bottom;
    tx=pVsp->Width()+(bPad?1:0); ty=pVsp->Height()+(bPad?1:0);
    
    x/=tx; y/=ty;
    
    uint t=(y+nYoffset)*(width/tx)+x;
    if (t<pVsp->NumTiles())
        return  t;
    else
        return -1;
}

void CTileSel::ScrollRel(int newoffset)
{
    Scroll(nYoffset+newoffset);
}

void CTileSel::Scroll(int newoffset)
{
    RECT temprect;
    int width,height;
    int tx,ty;
    
    if (!pVsp) return;

    GetClientRect(hWnd,&temprect);
    width=temprect.right; height=temprect.bottom;
    tx=pVsp->Width()+(bPad?1:0); ty=pVsp->Height()+(bPad?1:0);

    int tilesperrow;
    int tilerowsonscreen;
    int numtilerows;
    
    tilesperrow=width/tx;
    tilerowsonscreen=height/ty-1;
    numtilerows=pVsp->NumTiles()/tilesperrow+2;

    if (newoffset>numtilerows-tilerowsonscreen) newoffset=numtilerows-tilerowsonscreen;
    if (newoffset<0) newoffset=0;
		  
    nYoffset=newoffset;
		  
    // now that we know where the viewport is, let's adjust the scrollbar accordingly
    SCROLLINFO si;
		  
    si.cbSize=sizeof si;
    si.fMask=SIF_RANGE|SIF_PAGE|SIF_POS;
    si.nPage=tilerowsonscreen;
    si.nMin=0;
    si.nPos=newoffset;
    si.nMax=numtilerows;
    SetScrollInfo(hWnd,SB_VERT,&si,true);
}

void CTileSel::Execute(HINSTANCE hInst,HWND hParentWnd, VSP& v)
{
    if (!this) return;														// O_O;
    
    if (!hWnd)
    {
        hWnd=CreateDialog(hInst,"TileSelDlg",hParentWnd,CTileSel::WndProc);
        hMenu=LoadMenu(hInst,MAKEINTRESOURCE(IDR_ALTMENU));
        pGraph=new CGraphView(hWnd,CTileSel::RenderCallback,(void*)this);
    }

    pVsp=&v;

    SetWindowLong(hWnd,GWL_USERDATA,(long)this);							// the dialog we just created belongs to this object
    ShowWindow(hWnd,SW_SHOW);
    hWndParent=hParentWnd;
    hInstance=hInst;

    active=true;
    pGraph->ForceShowPage();
}

void CTileSel::Close()
{
    ShowWindow(hWnd,SW_HIDE);
    active=false;
}

void CTileSel::Render(const RECT& r)
{
    if (!active) return;
    if (!pVsp) return;
    
    int width,height;
    RECT clientrect;
    
    GetClientRect(hWnd,&clientrect);
    
    int tx=pVsp->Width()+(bPad?1:0);
    int ty=pVsp->Height()+(bPad?1:0);
    
    pGraph->Clear();	
    
    width=clientrect.right; height=clientrect.bottom;
    
    // This is quite inefficient.  Instead of figuring out which tiles are going to be actually redrawn, we simply let the blit method clip it.
    // Shouldn't be a problem, though.
    for (int y=0; y<height / ty; y++)
        for (int x=0; x<width / tx; x++)
        {
            uint nTile=(y+nYoffset)*(width/tx)+x;
            
            if (nTile>pVsp->NumTiles())
                return;
            
            pGraph->Blit(pVsp->GetTile(nTile),
                x*tx-r.left,
                y*ty-r.top,
                false);
        }
}

void CTileSel::RenderCallback(void* pThis,const RECT& r)
{
    ((CTileSel*)pThis)->Render(r);
}

void CTileSel::SaveVSP()
{
    const char sTitle[]  = "Save VSP as...";
    const char sFilter[] = "v2 VSP\0*.VSP\0ika VSP\0*.VSP\0PNG image\0*.PNG\0";
    char sFilename[256];
    OPENFILENAME ofn;
    
    ZeroMemory(&ofn,sizeof ofn);
    ofn.hwndOwner=hWnd;
    ofn.lStructSize=sizeof ofn;
    ofn.hInstance=hInstance;
    ofn.lpstrTitle=sTitle;
    ofn.lpstrFilter=sFilter;
    ofn.lpstrFileTitle=sFilename;
    ofn.nMaxFileTitle=255;
    ofn.Flags=OFN_OVERWRITEPROMPT;
    
    if (!GetSaveFileName(&ofn))
        return;
    
    int result = 0;
    switch (ofn.nFilterIndex)
    {
        //		case 1:	result=pVsp->SaveOld(sFilename);	break;
        
    case 2:	result=pVsp->Save(sFilename);		break;
    case 3:	result=ExportPNG(sFilename);		break;
    }
    
    if (!result)												// uh... I duno what to do.  So I'll just cry and bitch.
        Log::Write("pVsp->SaveAs failed! ;_;");
}

// kudos to JL for this.  I just ported it from Python.
bool CTileSel::ExportPNG(const char* fname)
{
    const int nTilewidth = 18;
    int nTileheight = (pVsp->NumTiles() + nTilewidth - 1) / nTilewidth;
    int nPixwidth = nTilewidth * pVsp->Width();
    int nPixheight= nTileheight* pVsp->Height();
    
    Canvas bigImage;
    bigImage.Resize(nPixwidth,nPixheight);
    bigImage.Clear(RGBA(0,255,0,255));
    
    int currX = 0;
    int currY = 0;
    int tileSize = pVsp->Width() * pVsp->Height() * 4;
    
    for (uint i=0; i<pVsp->NumTiles(); i++)
    {
        CBlitter<Opaque>::Blit(pVsp->GetTile(i),bigImage,currX,currY);
        
        currX+=pVsp->Width();
        if (currX>=nPixwidth)
        {
            currX=0;
            currY+=pVsp->Height();
        }
    }

    bigImage.Save(fname);
    
    return true;
}

BOOL CTileSel::MainProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    int t;
    static int x,y;
    static Canvas copybuffer;
    
    switch(msg)
    {
    case WM_INITDIALOG:
    case WM_SHOWWINDOW: 
        Scroll(0); // reset the window
        PostMessage(hWnd,WM_PAINT,NULL,NULL);
        return true;
    case WM_PAINT: 
        pGraph->ForceShowPage();
        return false;
        
    case WM_RBUTTONDOWN: 
        x=LOWORD(lParam);   
        y=HIWORD(lParam)-nGayarbitraryconstant;
        if (TileUnderCursor(x,y)<0) 
            return false;
        
        POINT p;
        p.x=x; p.y=y;
        ClientToScreen(hWnd,&p);
        TrackPopupMenuEx(GetSubMenu(hMenu,0),TPM_RIGHTBUTTON,p.x,p.y+nGayarbitraryconstant,hWnd,NULL);
        return false;
        
    case WM_LBUTTONDOWN: 
        x=LOWORD(lParam);  
        y=HIWORD(lParam)-nGayarbitraryconstant;
        t=TileUnderCursor(x,y);
        if (t<0) return false;
        
        SendMessage(hWndParent,WMU_SETLTILE,t,0);				// Tell the main window to change the active left tile.
        return false;
        
    case WM_COMMAND: 
        t=TileUnderCursor(x,y);
        switch (LOWORD(wParam))
        {
        case ID_FILE_SAVEVSP:									// blech, save an ika vsp always for now. (TODO: tweax0r)
            pVsp->Save(pVsp->Name());
            return false;
        case ID_FILE_SAVEAS:
            SaveVSP();
            return false;
        case ID_FILE_IMPORTFROMIMAGE:
            {
                CImportVSPDlg importdlg;
                importdlg.Execute(hInstance,hWnd,pVsp);
                break;
            }
        case ID_EDIT_ANIMATIONDATA:
            {
                CVSPAnimDlg animdlg;
                animdlg.Execute(hInstance,hWnd,pVsp);
                return false;
            }

        case ID_EDIT_PAD:
            {
                bPad=!bPad;
                break;
            }

            // popup menu
        case ID_INSERTTILE:
            pVsp->InsertTile(t);
            break;
        case ID_DELETETILE:
            pVsp->DeleteTile(t);
            break;
        case ID_COPYTILE:
            pVsp->CopyTile(copybuffer,t);
            return false;
            
        case ID_PASTETILE:
            pVsp->PasteTile(copybuffer,t);
            break;
        case ID_TPASTETILE:
            pVsp->TPasteTile(copybuffer,t);
            break;
        case ID_IPASTETILE:
            pVsp->InsertTile(t);
            pVsp->PasteTile(copybuffer,t);
            break;
        case ID_EDITTILE:                           
            {
                CTileEd tileed;
                tileed.Execute(hInstance,hWnd,pVsp,t);
            }
            break;
        }
        PostMessage(hWnd,WM_PAINT,0,0);
        return false;
        
        case WM_VSCROLL: 
            switch (LOWORD(wParam))
            {
            case SB_BOTTOM:
                Scroll(65535); // all the way down!
            case SB_TOP:
                Scroll(0);     // all the way up!
                break;
            case SB_LINEUP:
                ScrollRel(-1); // up one
                break;
            case SB_LINEDOWN:
                ScrollRel(1); // down one
                break;
            case SB_PAGEUP:
                ScrollRel(-10);
                break;
            case SB_PAGEDOWN:
                ScrollRel(10);
                break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
                Scroll(HIWORD(wParam));
                break;
            }
            PostMessage(hWnd,WM_PAINT,0,0);
            return false;
            
            case WM_SIZE:  
                switch(wParam)
                {
                case SIZE_MINIMIZED:
                case SIZE_MAXHIDE:
                    active=false;
                    return 0;
                case SIZE_MAXIMIZED:
                case SIZE_RESTORED:
                case SIZE_MAXSHOW: 
                    ScrollRel(0);								// make sure that we aren't mucking up the clipping as the graphics viewport changes size
                    active=true;
                    pGraph->ForceShowPage();
                    return 0;
                }
                
                case WM_CLOSE:
                    //tileed.Kill();
                    EndDialog(hWnd,IDOK);
                    return true;
        }
        return false;
}
