/*
STEEEEENKEY POO!!
*/

#include "main.h"

const int timerate=100;

void CEngine::Sys_Error(const char* errmsg)
{
    CDEBUG("sys_error");
    Shutdown();
    if (strlen(errmsg))
        MessageBox(hWnd,errmsg,"",0);
    PostQuitMessage(0);
    bKillFlag=true;
    exit(-1);                                            // SITE OF POTENTIAL EVILNESS!  Is this allowed in Win32?  Am I causing a horrible memory leak?
    return;
}

void CEngine::Script_Error()
{
    CDEBUG("script_error");
    Shutdown();
    
    File f;
    if (f.OpenRead("pyout.log"))
    {
        int nSize=f.Size();
        char* c=new char[nSize+1];
        ZeroMemory(c,nSize+1);
        f.Read(c,nSize);
        f.Close();
        MessageBox(hWnd,c,"Script error",0);
        delete[] c;
    }
    else
        MessageBox(hWnd,"Unknown error!","Script error",0);
    
    exit(-1);
    return;
}

int CEngine::CheckMessages()
{
    CDEBUG("checkmessages");
    MSG msg;
    
    if (bKillFlag) return 1;
    
    do
    {
        while (PeekMessage(&msg,NULL,0,0,PM_REMOVE))    // process things until the message queue is empty
        {         
            if (msg.message==WM_QUIT || msg.message==WM_CLOSE || (msg.message==WM_SYSCOMMAND && msg.wParam==SC_CLOSE))
            {
                bKillFlag=true;                            // so that everybody knows to stop what they're doing and quit
                Shutdown();
                exit(-1);                                // probably a bad idea, but maybe not, I'm not sure*/
                return 1;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!bActive)                                    // Are we ready to rock?
            WaitMessage();                                // Nope, wait for something to happen.
        
    } while (!bActive);
    
    return 0;
}

void CEngine::MainLoop()
{
    CDEBUG("mainloop");
    static int numframes,t,fps=0;                        // frame counter stuff
    
    while(1)
    {
        CheckMessages();
        
        int nFramesskipped=0;
        t+=timer.t;
        
        while (timer.t>0 && ++nFramesskipped<=cfg.nMaxframeskip)
        {
            timer.t--;
            
            if (!bKillFlag)
                GameTick();
            else
                return;
        }
        
        
        timer.t=0;
        
        if (t>timerate)                                    // update the frame counter
        {
            fps=numframes;
            numframes=0;
            t-=timerate;
            log("%i fps",fps);
        }
        numframes++;
        
        Render();
        
        //        font[0].PrintString(0,0,va("Fps: \\3%i",fps));
        
        gfxShowPage();
    }
}

void CEngine::Startup(HWND hwnd, HINSTANCE hinst)
// TODO: Make a nice happy GUI thingie for making a user.cfg
{
    CDEBUG("Startup");
    
    hWnd=hwnd;        hInst=hinst;
    
    initlog("ika.log");
    
    log("%s startup",VERSION);
    log("Built on %s",__DATE__);
    log("--------------------------");
    
    bKillFlag=false;
    
    cfg.Read("user.cfg");
    
    if (!SetUpGraphics(cfg.sGraphplugin))
    {
        Sys_Error("Unable to load graphics driver.");
        return;
    }
    
    logp("Initing graphics");
    bool a=gfxInit(hWnd,cfg.nInitxres,cfg.nInityres,cfg.nInitbpp,cfg.bFullscreen);
    if (!a)
    {
        Sys_Error("gfxInit failed.\nThis could mean that you're trying to set a video mode that your hardware cannot handle.");
        return;
    }
    logok();
    
    logp("Initing sound");
    a=SetupSound(cfg.bSound?cfg.sSoundplugin:"");
    if (!a)
        log("Sound initialization failed.  Disabling audio.");
    logok();
    
    logp("Initing input");
    if (!input.Init(hinst,hwnd))
    {
        Sys_Error("input.Init failed");
        return;
    }
    input.ClipMouse(0,0,cfg.nInitxres,cfg.nInityres);
    if (cfg.bFullscreen)        input.HideMouse();
    // Clear key bindings
    ZeroMemory(pBindings,nControls*sizeof(void*));                // It has come to my attention that I have far too few goofy comments in this source tree.
    logok();                                                    // I AM PEANUT
    
    logp("Initing timer");
    if (!timer.Init(timerate))
    {
        Sys_Error("timer.Init failed");
        return;
    }
    logok();
    
    
    bMaploaded=false;
    hRenderdest=gfxGetScreenImage();
    
    srand(timeGetTime());                                        // win32 specific x_x
    
    log("Initing Python");
    script.Init(this);
    log("Executing system.py");
    bool result=script.LoadSystemScripts("system.py");
    if (!result)
        Script_Error();
    
    if (!bMaploaded)
        Sys_Error("");
    
    log("Startup complete");
}

void CEngine::Shutdown()
{
    CDEBUG("shutdown");
    
#ifdef _DEBUG
    static bool blah=false;
    
    if (blah)
    {
        log("REDUNDANT CALLS TO SHUTDOWN!!!!!");
        return;
    }
    
    blah=true;
#endif
    
    log("---- shutdown ----");
    map.Free();
    timer.Shutdown();
    tiles.Free();
    //    sprite.Free();
    script.Shutdown();
    
    UnloadGraphics();
    ShutdownSound();
    input.ShutDown();
}

// ------------------------------------------------------------------------------------------

void CEngine::RenderEntities()
{
    CDEBUG("renderentities");
    std::vector<CEntity*>    drawlist;
    
    drawlist.clear();
    // first, get a list of entities onscreen
    int width,height;
    for (EntityIterator i=entities.begin(); i!=entities.end(); i++)
    {    
        CEntity& e=**i;
        CSprite& sprite=*e.pSprite;
        
        width =sprite.Width();
        height=sprite.Height();
        
        // get the coodinates at which the sprite would be drawn
        int x=e.x-sprite.nHotx;
        int y=e.y-sprite.nHoty;
        
        if (x+width-xwin>0                      && y+height-ywin>0 &&
            x-xwin<gfxImageWidth(hRenderdest)   && y-ywin<gfxImageHeight(hRenderdest) &&
            e.bVisible)
            drawlist.push_back(&e);                                                         // the entity is onscreen, tag it.
    }
    
    if (!drawlist.size())
        return;                                                                             // nobody onscreen?  Easy out!
    
    int nEntsonscreen=drawlist.size();
    bool blarg;
    do                                                                                      // quick n' dirty bubble sort
    {
        blarg=false;
        for (int i=0; i<nEntsonscreen; i++)
            for (int j=i+1; j<nEntsonscreen; j++)
                if (drawlist[i]->y>drawlist[j]->y)                                          // FIXME:  is this a gigantic bottleneck?  Quicksort?
                {
                    swap(drawlist[i],drawlist[j]);
                    blarg=true;
                }            
    } while (blarg);
    
    
    for (int j=0; j<nEntsonscreen; j++)
    {
        int hx,hy;
        CEntity& e=*drawlist[j];
        CSprite& s=*e.pSprite;;
        
        hx=s.nHotx;
        hy=s.nHoty;
        
        int frame=e.nSpecframe?e.nSpecframe:e.nCurframe;
        
        s.BlitFrame(e.x-xwin-s.nHotx, e.y-ywin-s.nHoty, frame);
    }
}

void CEngine::RenderLayer(int lay,bool transparent)
{
    CDEBUG("renderlayer");
    int        xl,yl;        // x/y run length
    int        xs,ys;        // x/y start
    int        xofs,yofs;    // sub-tile offset
    int        xw,yw;
    SMapLayerInfo    linf;
    
    if (lay>map.NumLayers())
        return;
    
    map.GetLayerInfo(linf,lay);
    
    xw=(xwin*linf.pmulx)/linf.pdivx;
    yw=(ywin*linf.pmulx)/linf.pdivy;
    
    xs=xw/tiles.Width();
    ys=yw/tiles.Height();
    
    xofs=-(xw%tiles.Width());
    yofs=-(yw%tiles.Height());
    
    xl= gfxImageWidth(hRenderdest)/tiles.Width()+1;
    yl=gfxImageHeight(hRenderdest)/tiles.Height()+2;
    
    if (xs+xl>map.Width()) xl=map.Height()-xs;        // clip yo
    if (ys+yl>map.Height()) yl=map.Height()-ys;
    
    u32*    t   =map.GetDataPtr(lay)+((ys)*map.Width() + xs);
    int        xinc=map.Width()-xl;
    
    int curx=xofs;
    int cury=yofs;
    if (transparent)
    {
        for (int y=0; y<yl; y++)
        {
            for (int x=0; x<xl; x++)
            {
                if (*t)
                    tiles.TBlitFrame(curx,cury,*t);
                curx+=tiles.Width();
                t++;
            }
            cury+=tiles.Height();
            curx=xofs;
            t+=xinc;
        }
    }
    else
    {
        for (int y=0; y<yl; y++)
        {
            for (int x=0; x<xl; x++)
            {
                tiles.BlitFrame(curx,cury,*t);
                curx+=tiles.Width();
                t++;
            }
            cury+=tiles.Height();
            curx=xofs;
            t+=xinc;
        }
    }
}

void CEngine::Render(const char* sTemprstring)
{
    CDEBUG("render");
    char    rstring[255];
    char*    p;
    int        numlayers;
    
    if (!bMaploaded)    return;
    
    tiles.UpdateAnimation(timer.systime);
    
    if (pCameratarget)
    {        
        xwin=pCameratarget->x- gfxImageWidth(hRenderdest)/2;                        // Move the camera...
        ywin=pCameratarget->y- gfxImageHeight(hRenderdest)/2;
        
        int maxx=(map.Width() *tiles.Width() ) - gfxImageWidth (hRenderdest);                // and make sure it's still in range
        int maxy=(map.Height()*tiles.Height()) - gfxImageHeight(hRenderdest);
        
        if (xwin<0)        xwin=0;
        if (ywin<0)        ywin=0;
        if (xwin>=maxx)    xwin=maxx-1;
        if (ywin>=maxy)    ywin=maxy-1;
    }
    
    if (!sTemprstring)
        strcpy(rstring,map.GetRString().c_str());
    else
        strcpy(rstring,sTemprstring);
    
    p=rstring;
    numlayers=0;
    //gfxRect(hRenderdest,0,0,320,240,0,true);
    
    while (*p)
    {
        switch (*p)
        {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            RenderLayer(*p-'1',numlayers?true:false);
            ++numlayers;
            break;
        case '0':
            RenderLayer(9,numlayers?true:false);
            ++numlayers;
            break;
        case 'E':
            RenderEntities();
            break;
        case 'R':
            HookRetrace();
            break;
        }
        ++p;
    }
}

void CEngine::HookTimer()
{
    if (pHooktimer.empty())
        return;
    
    std::list<void*>::iterator i;
    for (i=pHooktimer.begin(); i!=pHooktimer.end(); i++)
    {
        script.ExecFunction(*i);
    }
}

void CEngine::HookRetrace()
{
    if (pHookretrace.empty())
        return;
    
    std::list<void*>::iterator i;
    for (i=pHookretrace.begin(); i!=pHookretrace.end(); i++)
        script.ExecFunction(*i);
}

// ----------------------------------------- AI -------------------------------------------------

void CEngine::GameTick()
{
    CDEBUG("gametick");
    
    CheckKeyBindings();
    HookTimer();
    ProcessEntities();
}

void CEngine::CheckKeyBindings()
{
    int c;
    while ((c=input.NextControl())!=-1)    // while keys are in the queue
    {
        if (c<0 || c>nControls)
        {
            log("CEngine::CheckKeyBindings");
            return;
        }
        if (pBindings[c]!=NULL)
        {
            input.control[c]=0;
            script.ExecFunction(pBindings[c]);
            input.ClearControls();                // Not the perfect end result, but it'll have to do.  Don't want to call a script if one's already running
        }
    }
}

void CEngine::ProcessEntities()
{
    for (EntityIterator i=entities.begin(); i!=entities.end(); i++)
    {
        //        if (*i==pPlayer)
        //          continue;
        
        CEntity& ent=**i;
        
        ent.nSpeedcount+=ent.nSpeed;
        while (ent.nSpeedcount>=100)
        {
            ent.Update();
            ent.nSpeedcount-=100;
        }
    }
}

// --------------------------------------- Entity Handling --------------------------------------

int  CEngine::EntityAt(int x,int y,int w,int h)
// Returns the index of the entity within the specified rect
// Returns -1 if there is no entity there.
{
    CDEBUG("entityat");
    /*    CEntity* e;
    CSprite* s;
    
      for (int i=0; i<entities.Count(); i++)
      if (entities.IsValid(i))    
      {
      e=&entities[i];
      s=e->pSprite;
      
        if (e->x         >=x+w)     continue;
        if (e->y         >=y+h)     continue;
        if (e->x+s->nHotw<=x)       continue;
        if (e->y+s->nHoth<=y)       continue;
        return i;
}*/
    return -1;
}

bool CEngine::DetectMapCollision(int x,int y,int w,int h)
// returns true if there is an obstructed map square anywhere along a specified vertical or horizontal line
// Also TODO: think up a better obstruction system
{
    CDEBUG("detectmapcollision");
    int tx=tiles.Width();
    int ty=tiles.Height();
    
    if (h)
    {
        int y2=(y+h-1)/ty;
        
        x/=tiles.Width();
        y/=tiles.Height();
        for (int i=y; i<=y2; i++)
            if (map.IsObs(x,i))
                return true;
    }
    else if (w)
    {
        int x2=(x+w-1)/tx;
        
        x/=tiles.Width();
        y/=tiles.Height();
        for (int i=x; i<=x2; i++)
            if (map.IsObs(i,y))
                return true;
    }
    
    return false;
}

CEntity* CEngine::DetectEntityCollision(const CEntity& ent)
// returns the entity colliding with the specified entity, or 0 if none.
{
    CDEBUG("detectentitycollision");
    CSprite& sprite=*ent.pSprite;
    
    for (EntityIterator i=entities.begin(); i!=entities.end(); i++)
    {
        CEntity& e=**i;
        CSprite& s=*e.pSprite;

        if (&e==&ent || !e.bEntobs)             continue;
        
        if (ent.x               > e.x+s.nHotw)  continue;
        if (ent.y               > e.y+s.nHoth)  continue;
        if (ent.x+sprite.nHotw  < e.x)          continue;
        if (ent.y+sprite.nHoth  < e.y)          continue;
        
        return &e;
    }
    return 0;
}

Direction CEngine::HandlePlayer()
// Moves the player entity around according to the controls and whatnot
// Also checks for zone/entity activation
{
    CDEBUG("handleplayer");
    input.Update();
    
    TestActivate();
    
    if (input.up)    
    {
        if (input.left)        return face_upleft;
        if (input.right)    return face_upright;
        return face_up;
    }
    if (input.down)    
    {
        if (input.left)        return face_downleft;
        if (input.right)    return face_downright;
        return face_down;
    }
    if (input.left) return face_left;                    // by this point, the diagonal possibilities are already taken care of
    if (input.right)return face_right;
    
    return face_nothing;
}

Direction CEngine::HandleWanderingEntity(CEntity& ent)
{
/*    if (ent.movescriptct<1)
{
if (ent.thedirectionImgoinginnow==face_nothing)                    // are we through with a non-walking period?
{
ent.thedirectionImgoinginnow=face_down;//(Direction)(rand()%4);                            // 1-4 (up, down, left, or right)
ent.movescriptct=ent.nWandersteps;
return ent.thedirectionImgoinginnow;
}
else
{
ent.thedirectionImgoinginnow=face_nothing;
ent.movescriptct=ent.nWanderdelay;
return face_nothing;
}
}

  ent.movescriptct--;
  
    switch(ent.movecode)
    {
    case mc_wanderrect:
    switch(ent.thedirectionImgoinginnow)
    {
    case face_up:        if (ent.y-1<ent.wanderrect.top)            ent.thedirectionImgoinginnow=face_nothing;    break;
    case face_down:        if (ent.y+1>ent.wanderrect.bottom)        ent.thedirectionImgoinginnow=face_nothing;    break;
    case face_left:        if (ent.x-1<ent.wanderrect.left)        ent.thedirectionImgoinginnow=face_nothing;    break;
    case face_right:    if (ent.x+1>ent.wanderrect.right)        ent.thedirectionImgoinginnow=face_nothing;    break;
    
      case face_upleft:    if (ent.y-1<ent.wanderrect.top || ent.x-1<ent.wanderrect.left)    ent.thedirectionImgoinginnow=face_nothing;    break;
      case face_upright:    if (ent.y-1<ent.wanderrect.top || ent.x+1>ent.wanderrect.right)    ent.thedirectionImgoinginnow=face_nothing;    break;
      case face_downleft:    if (ent.y+1>ent.wanderrect.top || ent.x-1<ent.wanderrect.left)    ent.thedirectionImgoinginnow=face_nothing;    break;
      case face_downright:if (ent.y+1>ent.wanderrect.top || ent.x+1>ent.wanderrect.right)    ent.thedirectionImgoinginnow=face_nothing;    break;
      }
      break;
      }
      
        
    return ent.thedirectionImgoinginnow;*/
    return face_nothing;
}

#include <math.h>
Direction CEngine::HandleChasingEntity(CEntity& ent)
// TODO: Make this chase algorithm not suck. (Bresenham's line algorithm or something)
{
/*    int nDeltax,nDeltay;
int nTarget;
Direction d;
nTarget=ent.nEntchasetarget;

  nDeltax=entities[nTarget].x-ent.x;
  nDeltay=entities[nTarget].y-ent.y;
  
    int distance=(int)sqrt(nDeltax*nDeltax+nDeltay*nDeltay);
    if (distance<ent.nMinchasedist)    return face_nothing;    // The entity is close enough, and is thus content to be where it is.
    
      if (nDeltax<0) 
      {
      if (nDeltay<0)
      d=face_upleft;
      else if (nDeltay>0)
      d=face_downleft;
      else
      d=face_left;
      }
      else if (nDeltax>0)
      {
      if (nDeltay<0)
      d=face_upright;
      else if (nDeltay>0)
      d=face_downright;
      else
      d=face_right;
      }
      else if (nDeltay<0)
      d=face_up;
      else if (nDeltay>0)
      d=face_down;
      
    return d;*/
    return face_nothing;
}

void CEngine::TestActivate()
// checks to see if we're supposed to run some VC, due to the player's actions.
// Thus far, that's one of three things.
// 2) the player steps on a zone
// 1) the player talks to an entity
// 3) the player activates a zone whose aaa (has nothing to do with AA) property is set.

// This sucks.  It uses static varaibles, so it's not useful at all for any entity other than the player, among other things.
{
/*    CDEBUG("testactivate");
static int    nOldtx=-1;
static int    nOldty=-1;
static int    nOldzone=-1;
SMapZone zone;
CEntity& e=entities[player];
CSprite& s=*e.pSprite;

  int tx=(e.x+s.nHotw/2)/tiles.Width();
  int ty=(e.y+s.nHoth/2)/tiles.Height();
  
    int n=map.GetZone(tx,ty);
    // stepping on a zone?
    if ((tx!=nOldtx || ty!=nOldty) && n)                        // the player is not on the same zone it was before, check for activation
    {
    nOldtx=tx; nOldty=ty;                            // if we don't do this, the next processentities will cause the zone to be activated again and again and again...
    map.GetZoneInfo(zone,map.GetZone(tx,ty));
    if (((rand()%100) < zone.nActchance) && zone.sActscript.length())
    {
    script.CallEvent(zone.sActscript.c_str());
    input.enter=false;
    }
    }
    
      nOldtx=tx; nOldty=ty;
      
        // TODO: adjacent activation
        // This probably isn't the best place for that sort of thing.  Maybe in ProcessEntities, in the clause that
        // executes when an entity is obstructed.  That'd be both more efficient, and accurate.
        
          if (!input.enter) return;                                // From this point on, the only time we'd have to check this crap is if b1 was pressed.
          
            tx=e.x; ty=e.y;
            // entity activation
            switch(entities[player].facing)
            {
            case face_up:        ty-=s.nHoth;    break;
            case face_down:        ty+=s.nHoth;    break;
            case face_left:        tx-=s.nHotw;    break;
            case face_right:    tx+=s.nHotw;    break;
            
              case face_upleft:    tx-=s.nHotw;    ty-=s.nHoth;    break;
              case face_upright:    tx+=s.nHotw;    ty-=s.nHoth;    break;
              case face_downleft:    tx-=s.nHotw;    ty+=s.nHoth;    break;
              case face_downright:tx+=s.nHotw;    ty+=s.nHoth;    break;
              }
              
                int i=EntityAt(tx,ty,s.nHotw,s.nHoth);
                if (i>=0)
                {
                CEntity* e=&entities[i];
                
                  if (!e->bAdjacentactivate && e->sActscript.length()!=0)
                  {
                  script.CallEvent(e->sActscript.c_str());
                  input.enter=false;
                  return;
                  }
                  }
                  
                    // Activating a zone?
                    map.GetZoneInfo(zone,map.GetZone(tx/tiles.Width(),ty/tiles.Height()));
                    
                      if (zone.bAdjacentactivation)// && zone.script)
                      {
                      script.CallEvent(zone.sActscript.c_str());
                      input.enter=false;
}*/
}

CEntity* CEngine::SpawnEntity()
{
    CEntity* e=new CEntity(this);
    
    entities.push_back(e);
    
    return e;
}

void CEngine::DestroyEntity(CEntity* e)
{
    for (EntityIterator i=entities.begin(); i!=entities.end(); i++)
        if (e==(*i))
        {
            sprite.Free(e->pSprite);
            
            // important stuff, yo.  Need to find any existing pointers to this entity, and null them.
            if (pCameratarget==e)   pCameratarget=0;
            if (pPlayer==e)         pPlayer=0;
            
            // Sadly, this probably gets pretty slow.
            for (EntityIterator ii=entities.begin(); ii!=entities.end(); ii++)
            {
                CEntity& e=*(*ii);
                
                if (e.pChasetarget==(*i))
                    e.pChasetarget=0, e.movecode=mc_nothing;
                
                // TODO: add others, if needed
            }
            
            // actually nuke it
            entities.remove(*i);
            delete e;
            return;
        }
        
    // gwa!  In a Perfect Worlid his should never execute.
    log("Attempt to unallocate invalid entity!!!");
}

// --------------------------------- Misc (interface with old file formats, etc...) ----------------------

void CEngine::LoadMap(const char* filename)
/*
Most of the work involved here is storing the various parts of the v2-style map into memory under the new structure.
Golly, my first exception handling thingie. *sniffle*  They grow up so fast!
*/
{
    CDEBUG("loadmap");
    char    temp[255];
    char*   extension;
    
    try
    {
        log("Loading map \"%s\"",filename);
        
        if (!map.Load(filename)) throw filename;                            // actually load the map
        
        strcpy(temp,map.GetVSPName().c_str());                              // get the tileset name
        if (!tiles.LoadVSP(temp)) throw temp;                               // load a VSP
        
        script.ClearEntityList();                                           // DEI
        
        for (int i=0; i<map.NumEnts(); i++)
        {
            SMapEntity    ent;
            
            map.GetEntInfo(ent,i);
            
            CEntity* pEnt=new CEntity(this,ent);                            // convert the old entity struct into the new one.
            entities.push_back(pEnt);
            
            strcpy(temp,ent.sCHRname.c_str());
            extension=temp+strlen(temp)-3;                                  // get the extension
            
            pEnt->pSprite=sprite.Load(temp);                                // wee
            if (pEnt->pSprite==0)                                           // didn't load?
                Sys_Error(va("Unable to load CHR file \"%s\"",temp));       // wah
            
            script.AddEntityToList(pEnt);
        }
        
        xwin=ywin=0;                                                        // just in case
        bMaploaded=true;
        
        if (!script.LoadMapScripts(filename))
            Script_Error();
    }
    catch (const char* msg)
    {    Sys_Error(va("Failed to load %s",msg));    }
    catch (...)
    {    Sys_Error(va("Unknown error loading map %s",filename));    }
}

