
/*
Python/C++ interface
*/

#ifndef STDLIB_H
#define STDLIB_H

#define METHOD(x) PyObject* CScriptEngine::x (PyObject* self,PyObject* args)

#include "main.h"

static CEngine* pEngine;

// Prototyped structs and such:
struct v_ImageObject;
struct v_EntityObject;


METHOD(std_log)
{
    char* message;
    
    if (!PyArg_ParseTuple(args,"s:log",&message))
        return NULL;
    
    Log::Write(message);
    
    Py_INCREF(Py_None);
    return Py_None;                                    // returning void :)
}

METHOD(std_exit)
{
    char* message="";
    
    if (!PyArg_ParseTuple(args,"|s:exit",&message))
        return NULL;
    
    pEngine->Sys_Error(message);
    
    // never gets called... I hope.
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_delay)
{
    int ticks;
    
    if (!PyArg_ParseTuple(args,"i:delay",&ticks))
        return NULL;
    
    ticks+=pEngine->timer.systime;
    
    // Always check messages at least once.
    do
    if (pEngine->CheckMessages())
        return NULL;
    while (ticks>pEngine->timer.systime);
    
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_wait)
{
    int ticks;
    
    if (!PyArg_ParseTuple(args,"i:wait",&ticks))
        return NULL;
    
    CEntity* pSaveplayer=pEngine->pPlayer;
    pEngine->pPlayer=0;                                // stop the player entity
    
    pEngine->timer.t=0;
    
    ticks+=pEngine->timer.systime;
    while (ticks>pEngine->timer.systime)
    {
        if (pEngine->CheckMessages())
            return NULL;
        while (pEngine->timer.t>0)
        {
            pEngine->timer.t--;
            pEngine->GameTick();
        }
        pEngine->Render();
        gfxShowPage();
    }
    
    pEngine->pPlayer=pSaveplayer;                                // restore the player
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_gettime)
{
    if (!PyArg_ParseTuple(args,""))
        return NULL;
    
    return PyInt_FromLong((long)pEngine->timer.systime);
}

METHOD(std_random)
{
    int min,max;
    
    if (!PyArg_ParseTuple(args,"ii:Random",&min,&max))
        return NULL;
    
    return PyInt_FromLong(Random(min,max));
}

// video

METHOD(std_showpage)
{
    if (!PyArg_ParseTuple(args,""))
        return NULL;
    
    gfxShowPage();
    
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_getscreenimage)
{
    if (!PyArg_ParseTuple(args,""))
        return NULL;
    
    Py_INCREF(pScreenobject);
    return pScreenobject;
}

METHOD(std_palettemorph)
{
    int r=255,g=255,b=255;
    
    if (!PyArg_ParseTuple(args,"|iii:PaletteMorph",&r,&g,&b))
        return NULL;
    
    gfxPaletteMorph(r,g,b);
    
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_getrgb)
{
    int colour;
    
    if (!PyArg_ParseTuple(args,"i:GetRGB",&colour))
        return NULL;
    
    int b=colour&255;
    int g=(colour>>8)&255;
    int r=(colour>>16)&255;
    int a=(colour>>24)&255;
    
    return Py_BuildValue("iiii",r,g,b,a);
}

METHOD(std_rgb)
{
    int r,g,b,a=255;
    
    if (!PyArg_ParseTuple(args,"iii|i:RGB",&r,&g,&b,&a))
        return NULL;
    
    return PyInt_FromLong(RGBA(r,g,b,a).i);
}

METHOD(std_hookbutton)
{
    int nButton;
    PyObject* pFunc;
    
    if (!PyArg_ParseTuple(args,"iO:HookButton",&nButton,&pFunc))
        return NULL;
    
    if (pFunc==Py_None)
        pEngine->pBindings[nButton]=NULL;
    else
    {
        if (!PyCallable_Check(pFunc))
        {
            PyErr_SetString(PyExc_TypeError,"Hookbutton requires None or a function to be hooked");
            return NULL;
        }
        
        Py_INCREF(pFunc);
        Py_XDECREF((PyObject*)pEngine->pBindings[nButton]);
        
        pEngine->pBindings[nButton]=pFunc;
    }
    
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_hookretrace)
{
    PyObject*    pFunc;
    
    if (!PyArg_ParseTuple(args,"O:HookRetrace",&pFunc))
        return NULL;
    
    if (!PyCallable_Check(pFunc))
    {
        PyErr_SetString(PyExc_TypeError,"HookRetrace requires a function as a parameter");
        return NULL;
    }
    
    Py_INCREF(pFunc);
    pEngine->pHookretrace.Add(pFunc);
    
    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_unhookretrace)
{
    PyObject* pFunc=NULL;
    
    if (!PyArg_ParseTuple(args,"|O:UnhookRetrace",&pFunc))
        return NULL;
    
    if (!pFunc)
    {
        std::list<void*>::iterator i;
        
        for (i=pEngine->pHookretrace.begin(); i!=pEngine->pHookretrace.end(); i++)
            Py_DECREF((PyObject*)*i);                                        // dereference

        pEngine->pHookretrace.Clear();
    }
    else
    {
        std::list<void*>::iterator i;
        
        for (i=pEngine->pHookretrace.begin(); i!=pEngine->pHookretrace.end(); i++)
        {
            if (*i==pFunc)
            {
                Py_DECREF(pFunc);
                pEngine->pHookretrace.Remove(*i);
                break;
            }
        }
    }        

    Py_INCREF(Py_None);
    return Py_None;
}

METHOD(std_hooktimer)
{
    PyObject*    pFunc;
    
    if (!PyArg_ParseTuple(args,"O:HookTimer",&pFunc))
        return NULL;
    
    if (!PyCallable_Check(pFunc))
    {
        PyErr_SetString(PyExc_TypeError,"HookTimer requires a function as a parameter");
        return NULL;
    }
    
    Py_INCREF(pFunc);
//    pEngine->pHooktimer.push_back(pFunc);
    pEngine->pHooktimer.Add(pFunc);
    
    Py_INCREF(Py_None);
    return Py_None;
}


// GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY
METHOD(std_unhooktimer)
{
    PyObject* pFunc=NULL;
    
    if (!PyArg_ParseTuple(args,"|O:UnhookTimer",&pFunc))
        return NULL;
    
    if (!pFunc)
    {
        std::list<void*>::iterator i;
        
        for (i=pEngine->pHooktimer.begin(); i!=pEngine->pHooktimer.end(); i++)
            Py_DECREF((PyObject*)*i);                                    // dereference

        pEngine->pHooktimer.Clear();
    }
    else
    {
        std::list<void*>::iterator i;
        
        for (i=pEngine->pHooktimer.begin(); i!=pEngine->pHooktimer.end(); i++)
        {
            if (*i==pFunc)
            {
                Py_DECREF(pFunc);
                //pEngine->pHooktimer.remove(*i);
                pEngine->pHooktimer.Remove(*i);
                break;
            }
        }
    }
    Py_INCREF(Py_None);
    return Py_None;
}

// MASTA TABLE YO

// WAH WAH MACROS ARE EVIL WAH WAH WAH.
// I have no problem with this at all.  It's nice and clean looking.
#define FUNCTION(x,y) { x,CScriptEngine::y,1}

PyMethodDef standard_methods[] =
{
    //  name  | function
    
    // Misc
    FUNCTION("Log",std_log),
    FUNCTION("Exit",std_exit),
    FUNCTION("Delay",std_delay),
    FUNCTION("Wait",std_wait),
    FUNCTION("GetTime",std_gettime),
    FUNCTION("Random",std_random),
    
    // Video
    FUNCTION("LoadImage",std_loadimage),
    FUNCTION("ShowPage",std_showpage),
    FUNCTION("SetRenderDest",std_setrenderdest),
    FUNCTION("GetRenderDest",std_getrenderdest),
    FUNCTION("GetScreenImage",std_getscreenimage),
    FUNCTION("RGB",std_rgb),
    FUNCTION("GetRGB",std_getrgb),
    FUNCTION("PaletteMorph",std_palettemorph),
    
    // Entity
    FUNCTION("ProcessEntities",std_processentities),
    FUNCTION("SetPlayer",std_setplayer),
    FUNCTION("SetCameraTarget",std_setcameratarget),
    FUNCTION("GetCameraTarget",std_getcameratarget),
    FUNCTION("EntityAt",std_entityat),
    
    FUNCTION("HookButton",std_hookbutton),
    FUNCTION("HookRetrace",std_hookretrace),
    FUNCTION("UnhookRetrace",std_unhookretrace),
    FUNCTION("HookTimer",std_hooktimer),
    FUNCTION("UnhookTimer",std_unhooktimer),
    
    // Object constructors
    FUNCTION("Image",Image_New),
    FUNCTION("Music",Music_New),
    FUNCTION("Sound",Sound_New),
    FUNCTION("Font",Font_New),
    FUNCTION("Entity",std_spawnentity),
    {    NULL,    NULL    }                        // sentinel?  (end of list marker, I think)
};

#undef FUNCTION
#undef METHOD

#endif