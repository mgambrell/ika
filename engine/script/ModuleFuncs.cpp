
#include "ObjectDefs.h"
#include "main.h"
#include "timer.h"
#include "SDL/SDL.h"

#define METHOD(x)  PyObject* x(PyObject* self, PyObject* args)
#define METHOD1(x) PyObject* x(PyObject*)

namespace Script
{
    CEngine*    engine;

    PyObject*   entityDict;
    PyObject*   playerent;
    PyObject*   cameraTarget;

    PyObject*   sysModule;                         // the system scripts (system.py)
    PyObject*   mapModule;                         // scripts for the currently loaded map

    std::stringstream pyOutput;

    METHOD(std_log)
    {
        char* message;

        if (!PyArg_ParseTuple(args, "s:log", &message))
            return 0;

        Log::Write(message);

        Py_INCREF(Py_None);
        return Py_None;                                    // returning void :)
    }

    METHOD(std_exit)
    {
        char* message="";

        if (!PyArg_ParseTuple(args, "|s:exit", &message))
            return 0;

        engine->Sys_Error(message);

        // never gets called... I hope.
        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD1(std_getcaption)
    {
        char* s;
        SDL_WM_GetCaption(&s, 0);

        return PyString_FromString(s);
    }

    METHOD(std_setcaption)
    {
        char* s = "";

        if (!PyArg_ParseTuple(args, "|s:SetCaption", &s))
            return 0;

        SDL_WM_SetCaption(s, 0);

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD1(std_getframerate)
    {
        return PyInt_FromLong(engine->video->GetFrameRate());
    }

    METHOD(std_delay)
    {
        int ticks;

        if (!PyArg_ParseTuple(args, "i:delay", &ticks))
            return 0;

        int endtime = ticks + GetTime();

        // Always check messages at least once.
        do
        {
            engine->CheckMessages();
        }
        while (endtime > GetTime());

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD(std_wait)
    {
        int ticks;    
        if (!PyArg_ParseTuple(args, "i:wait", &ticks))
            return 0;

        ::Entity* pSaveplayer=engine->pPlayer;
        engine->pPlayer = 0;                             // stop the player entity

        int t = GetTime();
        int endtime = ticks + t;

        while (endtime > GetTime())
        {
            engine->CheckMessages();

            while (t < GetTime())
            {
                t++;
                engine->GameTick();
            }

            engine->Render();
            engine->video->ShowPage();
        }

        engine->pPlayer = pSaveplayer;                   // restore the player
        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD1(std_gettime)
    {
        return PyInt_FromLong((long)GetTime());
    }

    METHOD(std_random)
    {
        int min, max;

        if (!PyArg_ParseTuple(args, "ii:Random", &min, &max))
            return 0;

        return PyInt_FromLong(Random(min, max));
    }

    // video

    METHOD(std_getrgb)
    {
        int colour;

        if (!PyArg_ParseTuple(args, "i:GetRGB", &colour))
            return 0;

        int b=colour&255;
        int g=(colour>>8)&255;
        int r=(colour>>16)&255;
        int a=(colour>>24)&255;

        return Py_BuildValue("iiii", r, g, b, a);
    }

    METHOD(std_rgb)
    {
        int r, g, b, a=255;

        if (!PyArg_ParseTuple(args, "iii|i:RGB", &r, &g, &b, &a))
            return 0;

        return PyInt_FromLong(RGBA(r, g, b, a).i);
    }

    METHOD(std_processentities)
    {
        if (!PyArg_ParseTuple(args, ""))
            return 0;

        engine->ProcessEntities();

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD(std_setcameraTarget)
    {
        Script::Entity::EntityObject* ent;

        if (!PyArg_ParseTuple(args, "O:SetcameraTarget", &ent))
            return 0;

        if ((PyObject*)ent == Py_None)
        {
            engine->cameraTarget = 0;
            Py_XDECREF(cameraTarget);
            cameraTarget = 0;
        }
        else
        {
            if (ent->ob_type != &Script::Entity::type)
            {
                PyErr_SetString(PyExc_TypeError, "SetcameraTarget not called with entity/None object");
                return 0;
            }

            engine->cameraTarget = ent->ent;  // oops

            Py_INCREF(ent);
            Py_XDECREF(cameraTarget);
            cameraTarget=(PyObject*)ent;
        }

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD1(std_getcameraTarget)
    {
        PyObject* result = cameraTarget ? cameraTarget : Py_None;

        Py_INCREF(result);
        return result;
    }

    METHOD(std_setplayer)                                            // FIXME?  Is there a more intuitive way to do this?
    {
        Script::Entity::EntityObject* ent;

        if (!PyArg_ParseTuple(args, "O:SetPlayerEntity", &ent))
            return 0;

        if ((PyObject*)ent == Py_None)
        {
            Py_XDECREF(playerent);
            playerent = 0;
            engine->pPlayer = 0;
        }
        else
        {
            if (ent->ob_type != &Script::Entity::type)
            {
                PyErr_SetString(PyExc_TypeError, "SetPlayerEntity not called with entity object or None.");
                return 0;
            }

            Py_INCREF(ent);
            Py_XDECREF(playerent);
            playerent = (PyObject*)ent;

            engine->pPlayer = ent->ent;
        }

        PyObject* result = std_setcameraTarget(self, args);
        Py_XDECREF(result);

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD1(std_getplayer)
    {
        PyObject* result = playerent ? playerent : Py_None;
        Py_INCREF(result);
        return result;
    }

    METHOD(std_entityat)
    {
        int x, y, width, height;
        if (!PyArg_ParseTuple(args, "iiii|EntityAt", &x, &y, &width, &height))
            return 0;

        int x2=x+width;
        int y2=y+height;

        int count = 0;
        PyObject* pKey = 0;
        PyObject* pValue = 0;
        while (PyDict_Next(entityDict, &count, &pKey, &pValue))
        {
            if (pValue->ob_type != &Script::Entity::type)
            {
                Log::Write("Someone's being a goober and putting non-entities in ika.Map.entities! >:(");
                continue;
            }

            ::Entity* ent=((Script::Entity::EntityObject*)pValue)->ent;
            
            if (x>ent->x+ent->sprite->nHotw)    continue;
            if (y>ent->y+ent->sprite->nHoth)    continue;
            if (x2<ent->x)    continue;
            if (y2<ent->y)    continue;

            Py_INCREF(pValue);
            return pValue;
        }

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD(std_hookretrace)
    {
        PyObject*    pFunc;

        if (!PyArg_ParseTuple(args, "O:HookRetrace", &pFunc))
            return 0;

        if (!PyCallable_Check(pFunc))
        {
            PyErr_SetString(PyExc_TypeError, "HookRetrace requires a function as a parameter");
            return 0;
        }

        Py_INCREF(pFunc);
        engine->_hookRetrace.Add(pFunc);

        Py_INCREF(Py_None);
        return Py_None;
    }

    METHOD(std_unhookretrace)
    {
        PyObject* pFunc = 0;

        if (!PyArg_ParseTuple(args, "|O:UnhookRetrace", &pFunc))
            return 0;

        if (!pFunc)
        {
            engine->_hookRetrace.Clear();
        }
        else
        {
            HookList::List::iterator i;

            for (i = engine->_hookRetrace.begin(); i != engine->_hookRetrace.end();)
            {
                if (i->get() == pFunc)
                {
                    engine->_hookRetrace.Remove(i->get());
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

        if (!PyArg_ParseTuple(args, "O:HookTimer", &pFunc))
            return 0;

        if (!PyCallable_Check(pFunc))
        {
            PyErr_SetString(PyExc_TypeError, "HookTimer requires a function as a parameter");
            return 0;
        }

        Py_INCREF(pFunc);
        //    engine->_hookTimer.push_back(pFunc);
        engine->_hookTimer.Add(pFunc);

        Py_INCREF(Py_None);
        return Py_None;
    }


    // GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY GAY
    METHOD(std_unhooktimer)
    {
        PyObject* pFunc = 0;

        if (!PyArg_ParseTuple(args, "|O:UnhookTimer", &pFunc))
            return 0;

        if (!pFunc)
        {
            std::list<void*>::iterator i;

            engine->_hookTimer.Clear();
        }
        else
        {
            HookList::List::iterator i;

            for (i = engine->_hookTimer.begin(); i != engine->_hookTimer.end(); i++)
            {
                if (*i == pFunc)
                {
                    Py_DECREF(pFunc);
                    //engine->_hookTimer.remove(*i);
                    engine->_hookTimer.Remove(i->get());
                    break;
                }
            }
        }
        Py_INCREF(Py_None);
        return Py_None;
    }

    PyMethodDef standard_methods[] =
    {
        //  name  | function

        // Misc
        { "Log",            (PyCFunction)std_log,               METH_VARARGS,
            "Log(message)\n\n"
            "Writes a string to ika.log, if logging is enabled."
        },

        { "Exit",           (PyCFunction)std_exit,              METH_VARARGS,
            "Exit([message])\n\n"
            "Exits ika immediately, displaying the message onscreen, if specified."
        },

        { "GetCaption",     (PyCFunction)std_getcaption,        METH_NOARGS,
            "GetCaption() -> string\n\n"
            "Returns the caption on the ika window title bar."
        },

        { "SetCaption",     (PyCFunction)std_setcaption,        METH_VARARGS,
            "SetCaption(newcaption)\n\n"
            "Sets the caption on the ika window title bar."
        },

        { "GetFrameRate",   (PyCFunction)std_getframerate,      METH_NOARGS,
            "GetFrameRate() -> int\n\n"
            "Returns the current engine framerate, in frames per second."
        },

        { "Delay",          (PyCFunction)std_delay,             METH_VARARGS,
            "Delay(time)\n\n"
            "Freezes the engine for a number of 'ticks'. (one tick is 1/100th of a second)"
        },

        { "Wait",           (PyCFunction)std_wait,              METH_VARARGS,
            "Wait(time)\n\n"
            "Runs the engine for a number of ticks, disallowing player input.\n"
            "Unlike Delay, Wait causes entities to be processed, the tileset to be animated, and the map drawn."
        },

        { "GetTime",        (PyCFunction)std_gettime,           METH_NOARGS,
            "GetTime() -> int\n\n"
            "Returns the number of ticks since the engine was started."
        },

        { "Random",         (PyCFunction)std_random,            METH_VARARGS,
            "Random(min, max) -> int\n\n"
            "Returns a random integer less than or equal to min, and less than max.\n"
            "ie.  min <= value < max"
        },

        // Video

        { "RGB",            (PyCFunction)std_rgb,               METH_VARARGS,
            "RGB(r, g, b[, a]) -> int\n\n"
            "Creates a 32bpp colour value from the four colour levels passed.  If alpha is\n"
            "omitted, it is assumed to be 255. (opaque)"
        },

        { "GetRGB",         (PyCFunction)std_getrgb,            METH_VARARGS,
            "GetRGB(colour) -> tuple(int, int, int, int)\n\n"
            "Returns a 4-tuple containing the red, blue, green, and alpha values of the colour\n"
            "passed, respectively."
        },

        //{ "PaletteMorph", (PyCFunction)std_palettemorph,      METH_VARARGS },

        // Entity
        { "ProcessEntities", (PyCFunction)std_processentities,  METH_VARARGS,
            "ProcessEntities()\n\n"
            "Performs 1/100th of a second of entity AI.  Calling this 100 times a second\n"
            "will cause entities to move around as if the engine was in control."
        },

        { "SetCameraTarget", (PyCFunction)std_setcameraTarget,  METH_VARARGS,
            "SetCameraTarget(entity)\n\n"
            "Sets the camera target to the entity specified.  If None is passed instead, \n"
            "the camera remains stationary, and can be altered with the Map.xwin and Map.ywin\n"
            "properties."
        },

        { "GetCameraTarget", (PyCFunction)std_getcameraTarget,  METH_NOARGS,
            "GetCameraTarget() -> Entity\n\n"
            "Returns the entity that the camera is following, or None if it is free."
        },

        { "SetPlayer",      (PyCFunction)std_setplayer,         METH_VARARGS,
            "SetPlayer(entity)\n\n"
            "Sets the player entity to the entity passed.  The player entity is the entity\n"
            "that moves according to user input.  Passing None instead unsets any player entity\n"
            "that may have been previously set."
        },

        { "GetPlayer",      (PyCFunction)std_getplayer,         METH_NOARGS,
            "GetPlayer(entity) -> Entity\n\n"
            "Returns the current player entity, or None if there isn't one."
        },
        { "EntityAt",       (PyCFunction)std_entityat,          METH_VARARGS,
            "EntityAt(x, y, width, height) -> Entity\n\n"
            "If there is an entity within the rectangle passed, it is returned.\n"
            "If not, then None is returned."
        },

        { "HookRetrace",    (PyCFunction)std_hookretrace,       METH_VARARGS,
            "HookRetrace(function)\n\n"
            "Adds the function to the retrace queue. (it will be called whenever the map is drawn, \n"
            "whether by Map.Render or by other means)"
        },

        { "UnhookRetrace",  (PyCFunction)std_unhookretrace,     METH_VARARGS,
            "UnhookRetrace([function])\n\n"
            "Removes the function from the retrace queue if it is present.  If not, the call does\n"
            "nothing.  If the argument is omitted, then the list is cleared in its entirety."
        },

        { "HookTimer",      (PyCFunction)std_hooktimer,         METH_VARARGS,
            "HookTimer(function)\n\n"
            "Adds the function to the timer queue. (the function will be called 100 times per second.\n"
            "This feature should be used sparingly, as it will cause serious problems if the queue\n"
            "cannot be executed in less than 1/100th of a second."
        },

        { "UnhookTimer",    (PyCFunction)std_unhooktimer,       METH_VARARGS,
            "UnhookTimer([function])\n\n"
            "Removes the function from the timer queue if it is present.  If not, the call does\n"
            "nothing.  If the argument is omitted, then the list is cleared in its entirety."
        },

        {    0    }
    };

#undef METHOD
#undef METHOD1
}
