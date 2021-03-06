
#include "wx/event.h"

#include "fontview.h"
#include "main.h"

namespace
{
    class FontFrame : public GraphicsFrame
    {
        DECLARE_EVENT_TABLE()

        FontView* pFontview;

    public:
        FontFrame(wxWindow* parent, FontView* fontview)
            : GraphicsFrame(parent)
            , pFontview(fontview)
        {}

        void OnPaint(wxPaintEvent& event)
        {
            wxPaintDC dc(this);

            pFontview->Paint();
        }
    };

    BEGIN_EVENT_TABLE(FontFrame, GraphicsFrame)
        EVT_PAINT(FontFrame::OnPaint)
    END_EVENT_TABLE()
}

BEGIN_EVENT_TABLE(FontView, DocumentPanel)

    EVT_SCROLLWIN(FontView::OnScroll)
    EVT_CLOSE(FontView::OnClose)
    EVT_ERASE_BACKGROUND(FontView::OnEraseBackground)

    EVT_LEFT_DOWN(FontView::OnLeftClick)
    EVT_RIGHT_DOWN(FontView::OnRightClick)

    EVT_MENU(FontView::id_filesave, FontView::OnSave)
    EVT_MENU(FontView::id_filesaveas, FontView::OnSaveAs)
    EVT_MENU(FontView::id_optionscolor, FontView::OnChangeBackgroundColor)

END_EVENT_TABLE()

FontView::FontView(MainWindow* parentwnd, const std::string& fname)
    : DocumentPanel(parentwnd, fname)
    , pParent(parentwnd)
    , sFilename(fname)
    , nCurfont(0)
    , ywin(0)
{

    pGraph = new FontFrame(this, this);
    pGraph->SetSize(GetClientSize());

    pFontfile = new FontFile();
    pFontfile->Load(sFilename.c_str());

    wxMenuBar* menubar = pParent->CreateBasicMenu();
    wxMenu* filemenu = menubar->Remove(0);
    filemenu->InsertSeparator(2);
    filemenu->Insert(3, new wxMenuItem(filemenu, id_filesave, "&Save", "Save the font to disk."));
    filemenu->Insert(4, new wxMenuItem(filemenu, id_filesaveas, "Save &As", "Save the font under a new filename."));
    filemenu->Insert(5, new wxMenuItem(filemenu, id_fileclose, "&Close", "Close the font window."));
    menubar->Append(filemenu, "&File");

    wxMenu* optionsmenu = new wxMenu;
    menubar->Append(optionsmenu, "&Options");
    optionsmenu->Append(id_optionscolor, "Change background color...", "");

    SetMenuBar(menubar);
}

FontView::~FontView()
{
//    pParent->font.Release(pFontfile);
    delete pFontfile;
}
void FontView::OnRightClick(wxMouseEvent& event)
{
}

void FontView::OnLeftClick(wxMouseEvent& event)
{
    int x, y;
    event.GetPosition(&x, &y);
    nCurfont = FontAt(x, y);

    Render();
}


void FontView::Render()
{

    int tx = 0, ty = 0;
    int nWidth, nHeight;

    GetClientSize(&nWidth, &nHeight);

    tx = pFontfile->Width();
    ty = pFontfile->Height();

    int nFontwidth = nWidth / tx;
    int nFontheight = (nHeight / ty) + 1;
    int nFont = ywin * nFontwidth;

    pGraph->SetCurrent();
    pGraph->Clear();     

    for(int y = 0; y < nFontheight; y++)
    {
        for(int x = 0; x < nFontwidth; x++)
        {
            // Grab the font bitmap, blit, and move right along.
            //  -- khross

            // Too slow. -- andy
            Canvas& rBitmap = pFontfile->GetGlyph(nFont);
            Image rImage(rBitmap);
            pGraph->ScaleBlit(rImage, x * tx + 1, y * ty + 1,
                rBitmap.Width(), rBitmap.Height(), true);

            // TODO: add zooming.

            nFont++;

            if (nFont >= pFontfile->NumGlyphs()) 
                goto nomoredrawing; // abort the loop

            nFontwidth = nWidth / tx;
            nFontheight = (nHeight / ty) + 1;

        }
    }
nomoredrawing:

    int x2, y2;
    tx = pFontfile->Width();
    ty = pFontfile->Height();

    FontPos(nCurfont, x2, y2);
    pGraph->Rect(x2 - 1, y2 - 1, tx + 1, ty + 1, RGBA(255, 255, 255));
    pGraph->ShowPage();
}

void FontView::FontPos(int fontidx, int& x, int& y) const
{
    int nFontwidth = GetClientSize().GetWidth()/pFontfile->Width();
    

    x = fontidx%nFontwidth;
    y = fontidx / nFontwidth - ywin;

    x *= pFontfile->Width();
    y *= pFontfile->Height();
}

int FontView::FontAt(int x, int y) const
{
    const int tx = pFontfile->Width();
    const int ty = pFontfile->Height();

    int nFontwidth = GetClientSize().GetWidth()/tx;

    x /= tx;      
    y /= ty;

    int t=(y + ywin)*nFontwidth + x;

    if (t > pFontfile->NumGlyphs()) return 0;
    return t;
}

void FontView::Paint()
{
    if (!pFontfile)
        return; // Can't be too careful with these wacky paint messages. -- khross

    Render();
}

void FontView::OnSave(wxCommandEvent& event)
{
    // FIXME: FontFile::Save isn't implemented. :x
    pFontfile->Save(sFilename.c_str());
}

void FontView::OnSaveAs(wxCommandEvent& event)
{
    wxFileDialog dlg
    (
        this,
        "Open File",
        "",
        "",
        "Font files (*.fnt)|*.fnt|"
        "All files (*.*)|*.*",
        wxSAVE | wxOVERWRITE_PROMPT
    );

    int result = dlg.ShowModal();
    if (result==wxID_CANCEL)
        return;

    name = dlg.GetFilename().c_str();
    SetTitle(name.c_str());

    OnSave(event);
}

const void* FontView::GetResource() const
{
    return pFontfile;
}

void FontView::OnClose()
{
    Destroy();
}

void FontView::OnSize(wxSizeEvent& event)
{
    pGraph->SetSize(GetClientSize());
    UpdateScrollbar();
}

void FontView::OnScroll(wxScrollWinEvent& event)
{
    if (event.m_eventType == wxEVT_SCROLLWIN_TOP)           ywin = 0;
    else if (event.m_eventType == wxEVT_SCROLLWIN_BOTTOM)   ywin = pFontfile->NumGlyphs();
    else if (event.m_eventType == wxEVT_SCROLLWIN_LINEUP)   ywin--;
    else if (event.m_eventType == wxEVT_SCROLLWIN_LINEDOWN) ywin++;
    else if (event.m_eventType == wxEVT_SCROLLWIN_PAGEUP)   ywin -= GetScrollThumb(wxVERTICAL);
    else if (event.m_eventType == wxEVT_SCROLLWIN_PAGEDOWN) ywin += GetScrollThumb(wxVERTICAL);
    else                                                    ywin = event.GetPosition();

    UpdateScrollbar();
    Render();
}

void FontView::UpdateScrollbar()
{
    Canvas& rGlyph = pFontfile->GetGlyph(nCurfont);
    int nWidth, nHeight;
    GetClientSize(&nWidth, &nHeight);

    int nFontwidth  = nWidth / rGlyph.Width();
    int nFontheight = nHeight / rGlyph.Height();

    int nTotalheight=(pFontfile->NumGlyphs())/nFontwidth;

    if (ywin > nTotalheight - nFontheight)  
        ywin = nTotalheight - nFontheight;

    if (ywin < 0)                         
        ywin = 0;

    SetScrollbar(wxVERTICAL, ywin, nFontheight, nTotalheight, true);
}

void FontView::OnChangeBackgroundColor(wxCommandEvent& event)
{
        wxColour nColor;
        nColor = GetBackgroundColour();
        
        wxColourData hData;
        hData.SetColour(nColor);
        hData.SetChooseFull(false);

        for (int i = 0; i < 16; i++)
        {
            wxColour nCustom(i * 16, i * 16, i * 16);
            hData.SetCustomColour(i, nCustom);
        }

        wxColourDialog cdialog(this, &hData);
        cdialog.SetTitle("Choose the background color");
        if (cdialog.ShowModal() == wxID_OK)
        {
            wxColourData retData = cdialog.GetColourData();
            nColor               = retData.GetColour();
            SetBackgroundColour(nColor);

            u8 r = nColor.Red();
            u8 g = nColor.Green();
            u8 b = nColor.Blue();

            glClearColor(r, g, b, 0);
            Render();
            
        }
}