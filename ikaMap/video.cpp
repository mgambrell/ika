
#include "video.h"

//-------------------------------------------------------

BEGIN_EVENT_TABLE(VideoFrame, wxGLCanvas)
    EVT_ERASE_BACKGROUND(VideoFrame::OnErase)
    EVT_SIZE(VideoFrame::OnSize)

    // VideoFrames send all mouse events to their parent window, after adapting for zoom
    EVT_MOUSE_EVENTS(VideoFrame::OnMouseEvent)
END_EVENT_TABLE()

static void SetTex(uint tex)
{
    static uint last = 0;

    if (tex == last)
        return;

    glBindTexture(GL_TEXTURE_2D, tex);
    last = tex;
}

std::set<VideoFrame*> VideoFrame::_instances;

VideoFrame::VideoFrame(wxWindow* parent)
    : wxGLCanvas(parent, _instances.empty() ? (wxGLCanvas*)0 : *_instances.begin())
    , _curZoom(16)
{
    int w, h;
    GetClientSize(&w, &h);

    Show();

    SetCurrent();
    glClearColor(0, 0, 0, 0);
    glClearDepth(1);

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    _instances.insert(this);
}

VideoFrame::~VideoFrame() {
    int q = _instances.size();
    _instances.erase(this);
    q = _instances.size();
}

void VideoFrame::OnSize(wxSizeEvent& event) {
    int w = event.GetSize().GetWidth();
    int h = event.GetSize().GetHeight();

    SetSize(w, h);
    SetCurrent();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
    glViewport(0, 0, w, h);

    glScissor(0, 0, w, h);
}

void VideoFrame::OnMouseEvent(wxMouseEvent& event) {
    // Tweak the mouse position, so that the parent doesn't have to compensate for interacting with the frame.
    event.m_x = event.m_x * _curZoom / nZoomscale;
    event.m_y = event.m_y * _curZoom / nZoomscale;

    // Relay to the parent.
    wxPostEvent(GetParent(), event);
}

void VideoFrame::DrawRect(int x, int y, int w, int h, RGBA colour) {
    x = x * nZoomscale / _curZoom;
    y = y * nZoomscale / _curZoom;
    w = w * nZoomscale / _curZoom;
    h = h * nZoomscale / _curZoom;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glColor4ub(colour.r, colour.g, colour.b, colour.a);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    glTranslatef(0.375f, 0.375f, 0);

    glBegin(GL_LINE_LOOP);

    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);

    glEnd();

    glPopMatrix();
    glPopAttrib();
}

void VideoFrame::DrawRectFill(int x, int y, int w, int h, RGBA colour) {
    x = x * nZoomscale / _curZoom;
    y = y * nZoomscale / _curZoom;
    w = w * nZoomscale / _curZoom;
    h = h * nZoomscale / _curZoom;

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT);
    glColor4ub(colour.r, colour.g, colour.b, colour.a);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    //glTranslatef(0.375f, 0.375f, 0);

    glBegin(GL_QUADS);

    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);

    glEnd();

    glPopMatrix();
    glPopAttrib();
}


void VideoFrame::DrawSelectRect(int x, int y, int w, int h, RGBA colour) {
    if (w < 0) {
        x += w;
        w -= w * 2;
    }

    if (h < 0) {
        y += h;
        h -= h * 2;
    }

    DrawRect(x, y, w, h, RGBA(0, 0, 0));
    DrawRect(x + 1, y + 1, w - 2, h - 2, colour);
    DrawRect(x + 2, y + 2, w - 4, h - 4, RGBA(255, 255, 255));
    DrawRect(x + 3, y + 3, w - 6, h - 6, RGBA(0, 0, 0));
}

void VideoFrame::Blit(Image& src, int x, int y, bool trans) {
    ScaleBlit(src, x, y, src._width, src._height, trans);
}

void VideoFrame::ScaleBlit(Image& src, int x, int y, int w, int h, bool trans) {
    x = x * nZoomscale / _curZoom;
    y = y * nZoomscale / _curZoom;
    w = w * nZoomscale / _curZoom;
    h = h * nZoomscale / _curZoom;

    if (_curZoom != nZoomscale) {
        w++; h++;
    }

    GLfloat nTexendx = 1.0f * src._width / src._texWidth;
    GLfloat nTexendy = 1.0f * src._height / src._texHeight;

    SetTex(src._handle);
    if (trans) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    glBegin(GL_QUADS);

    glTexCoord2f(       0,       0);    glVertex2i(x, y);
    glTexCoord2f(nTexendx,       0);    glVertex2i(x + w, y);
    glTexCoord2f(nTexendx, nTexendy);   glVertex2i(x + w, y + h);
    glTexCoord2f(       0, nTexendy);   glVertex2i(x, y + h);

    glEnd();
}

void VideoFrame::Clear() {
    glClear(GL_COLOR_BUFFER_BIT);
}

void VideoFrame::SetClearColour(RGBA colour) {
    SetCurrent();
    glClearColor(colour.r / 255.0, colour.g / 255.0, colour.b / 255.0, colour.a / 255.0);
}

void VideoFrame::ShowPage() {
    SwapBuffers();
}

int VideoFrame::LogicalWidth() const {
    return GetClientSize().GetWidth() * _curZoom / nZoomscale;
}

int VideoFrame::LogicalHeight() const {
    return GetClientSize().GetHeight() * _curZoom / nZoomscale;
}

int VideoFrame::GetZoom() const {
    return _curZoom;
}

void VideoFrame::SetZoom(int z) {
    _curZoom = max(1, z);
}

void VideoFrame::IncZoom(int amt) {
    _curZoom = max(1, _curZoom + amt);
}

Image::Image(const Canvas& src) {
    glGenTextures(1, &_handle);
    Update(src);
}

Image::~Image() {
    SetTex(0);
    glDeleteTextures(1, &_handle);
}

void Image::Update(const Canvas& src) {
    _width = src.Width();
    _height = src.Height();

#if 1
    _texWidth = nextPowerOf2(_width);
    _texHeight = nextPowerOf2(_height);
#else
    _texWidth = 1;
    _texHeight = 1;

    while (_texWidth < _width) _texWidth <<= 1;
    while (_texHeight < _height) _texHeight <<= 1;
#endif

    Canvas tmp(src);
    tmp.Resize(_texWidth, _texHeight);

    SetTex(_handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, _texWidth, _texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, (u32*)tmp.GetPixels());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}
