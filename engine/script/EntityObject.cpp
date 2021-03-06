/*
 * Python entity interface.
 *
 * I'm not sure if I'm %100 happy with this setup.  Entities are *only* destroyed through Python's GC, which means that
 * entities can be made "persistent" simply by holding on to a reference to them.  Additionally, there is an implicitly
 * defined dictionary that holds all the map-defined entities. (and serves as their "life line", to keep them from
 * being garbage collected)
 */

#include "ObjectDefs.h"
#include "main.h"
#include "entity.h"

#include <set>
#include <cassert>
#include <stdexcept>

namespace Script {
    namespace Entity {
        typedef std::map< ::Entity*, Script::Entity::EntityObject*> EntityMap;
        typedef std::pair< ::Entity*, Script::Entity::EntityObject*> EntityPair;

        // Maps all existing Entity objects to their Python counterparts.
        EntityMap instances;

        PyTypeObject type;

        PyMethodDef methods [] = {
            {   "MoveTo",           (PyCFunction)Entity_MoveTo,            METH_VARARGS,
                "Entity.MoveTo(x, y)\n\n"
                "Directs the entity to move towards the position specified."
            },

            {   "Wait",            (PyCFunction)Entity_Wait,               METH_VARARGS,
                "Entity.Wait(time)\n\n"
                "Causes the entity to halt for the given interval before\n"
                "resuming motion."
            },

            {   "Stop",            (PyCFunction)Entity_Stop,               METH_VARARGS,
                "Entity.Stop()\n\n"
                "Directs the entity to stop whatever it is doing."
            },

            {   "IsMoving",        (PyCFunction)Entity_IsMoving,           METH_VARARGS,
                "Entity.IsMoving() -> int\n\n"
                "If the entity is moving, the result is 1.  If not, it is 0."
            },

            {   "DetectCollision", (PyCFunction)Entity_DetectCollision,    METH_VARARGS,
                "Entity.DetectCollision() -> Entity\n\n"
                "If an entity is touching the entity, then it is returned.\n"
                "None is returned if there is no entity touching it."
            },

            {   "Touches",          (PyCFunction)Entity_Touches,            METH_VARARGS,
                "Touches(ent) -> boolean\n\n"
                "Returns true if the entity is touching entity ent, else False."
            },

            {   "Render", (PyCFunction)Entity_Render,                         METH_VARARGS,
                "Entity.Render()\n\n"
                "Draws the entity at the position specified.  Unlike Entity.Draw,\n"
                "Entity.Render() will call a render script if one is set.\n\n"
                "For this reason, *DO NOT CALL THIS IN A RENDER SCRIPT*.  Puppies\n"
                "will explode, and you will be very sad."
            },

            {   "Draw", (PyCFunction)Entity_Draw,                           METH_VARARGS,
                "Entity.Draw([x[, y[, frame]]])\n\n"
                "Draws the entity at the position specified.  x and y default to\n"
                "the position where they would normally draw, given the window position\n"
                "and the entity position.\n\n"
                "Render scripts are ignored by this method, and can easily be used within\n"
                "them without hassle."
            },
            {   "Update", (PyCFunction)Entity_Update,                      METH_VARARGS,
                "Entity.Update()\n\n"
                
                "Performs 1/100th of a second of entity AI. Calling this 100 times a second.\n"
                "will cause the entity to move around as if the engine was in control."
            },

            {   "GetAnimScript",    (PyCFunction)Entity_GetAnimScript,      METH_VARARGS,
                "Entity.GetAnimScript(name ->str) -> str\n\n"
                "Returns the animation script with the name given, for the entity's current\n"
                "spriteset.  The empty string is returned if no such script exists."
            },

            {   "GetAllAnimScripts",    (PyCFunction)Entity_GetAllAnimScripts,  METH_VARARGS,
                "Entity.GetAllAnimScripts() -> dict\n\n"
                "Returns a dict containing all animation scripts defined in the entity's current\n"
                "spriteset."
            },

            {    0    },                        // end of list
        };

#define GET(x) PyObject* get ## x(EntityObject* self)
#define SET(x) PyObject* set ## x(EntityObject* self, PyObject* value)
            GET(X)                  { return PyInt_FromLong(self->ent->x); }
            GET(Y)                  { return PyInt_FromLong(self->ent->y); }
            GET(Layer)              { return PyInt_FromLong(self->ent->layerIndex); }
            GET(Speed)              { return PyInt_FromLong(self->ent->speed); }
            GET(Direction)          { return PyInt_FromLong(self->ent->direction); }
            GET(CurFrame)           { return PyInt_FromLong(self->ent->curFrame); }
            GET(SpecFrame)          { return PyInt_FromLong(self->ent->specFrame); }
            GET(SpecAnim)
            {
                if (self->ent->useSpecAnim) {
                    return PyString_FromString(self->ent->specAnim.toString().c_str());
                } else {
                    Py_INCREF(Py_None);
                    return Py_None;
                }
            }

            GET(Visible)            { return PyInt_FromLong(self->ent->isVisible ? 1 : 0); }
            GET(Name)               { return PyString_FromString(self->ent->name.c_str()); }

            GET(MoveScript) {
                PyObject* o = (PyObject*)self->ent->moveScript.get();
                if (!o) {
                    o = Py_None;
                }
                Py_INCREF(o);
                return o;
            }

            GET(RenderScript) {
                PyObject* o = (PyObject*)self->ent->renderScript.get();
                if (o == 0) o = Py_None;
                Py_INCREF(o);
                return o;
            }

            GET(ActScript) {
                PyObject* o = (PyObject*)self->ent->activateScript.get();
                if (!o) {
                    o = Py_None;
                }
                Py_INCREF(o);
                return o;
            }

            GET(AdjacentActivate) {
                PyObject* o = (PyObject*)self->ent->adjActivateScript.get();
                if (!o) {
                    o = Py_None;
                }
                Py_INCREF(o);
                return o;
            }

//            GET(AutoFace)           { return PyInt_FromLong(self->ent->bAutoface?1:0; }
            GET(IsObs)              { return PyInt_FromLong(self->ent->obstructsEntities?1:0); }
            GET(MapObs)             { return PyInt_FromLong(self->ent->obstructedByMap?1:0); }
            GET(EntObs)             { return PyInt_FromLong(self->ent->obstructedByEntities?1:0); }
            GET(SpriteName)         { return PyString_FromString(self->ent->sprite->_fileName.c_str()); }
            GET(SpriteWidth)        { return PyInt_FromLong(self->ent->sprite->Width()); }
            GET(SpriteHeight)       { return PyInt_FromLong(self->ent->sprite->Height()); }
            GET(HotX)               { return PyInt_FromLong(self->ent->sprite->nHotx); }
            GET(HotY)               { return PyInt_FromLong(self->ent->sprite->nHoty); }
            GET(HotWidth)           { return PyInt_FromLong(self->ent->sprite->nHotw); }
            GET(HotHeight)          { return PyInt_FromLong(self->ent->sprite->nHoth); }

            SET(X)                  { self->ent->x = PyInt_AsLong(value); return 0; }
            SET(Y)                  { self->ent->y = PyInt_AsLong(value); return 0; }

            SET(Layer) {
                uint i = (uint)PyInt_AsLong(value);
                if (i >= engine->map.NumLayers()) {
                    PyErr_SetString(PyExc_RuntimeError,
                        va("Cannot put entity on layer %i.  The map only has %i layers.", i, engine->map.NumLayers())
                    );
                } else {
                    self->ent->layerIndex = i;
                }
                return 0;
            }

            SET(Speed)              { self->ent->speed = PyInt_AsLong(value); return 0; }

            SET(Direction) {
                self->ent->direction = (Direction)PyInt_AsLong(value);

                Direction dir = self->ent->direction;
                if (!self->ent->isMoving) {
                    self->ent->SetAnimScript(self->ent->sprite->GetWalkScript(dir));
                } else {
                    self->ent->SetAnimScript(self->ent->sprite->GetIdleScript(dir));
                }

                self->ent->UpdateAnimation();
                return 0;
            }

            SET(SpecFrame)          { self->ent->specFrame = PyInt_AsLong(value); return 0; }

            SET(SpecAnim) {
                if (value == Py_None) {
                    self->ent->useSpecAnim = false;
                } else {
                    self->ent->useSpecAnim = true;
                    self->ent->specAnim = AnimScript(PyString_AsString(value));
                }
                return 0;
            }

            SET(Visible)            { self->ent->isVisible = PyInt_AsLong(value)!=0 ; return 0; }
            SET(Name)               { self->ent->name = PyString_AsString(value); return 0; }
            SET(MoveScript) {
                self->ent->delayCount = 0;
                self->ent->moveScript.set(value);
                return 0;
            }

            SET(RenderScript) {
                self->ent->renderScript.set(value);
                return 0;
            }

            SET(ActScript) {
                self->ent->activateScript.set(value);
                return 0;
            }

            SET(AdjacentActivate) {
                self->ent->adjActivateScript.set(value);
                return 0;
            }

//            SET(AutoFace)           { self->ent->bAutoface = PyInt_AsLong(value) != 0; return 0; }
            SET(IsObs)              { self->ent->obstructsEntities = (PyInt_AsLong(value)!=0) ; return 0; }
            SET(MapObs)             { self->ent->obstructedByMap = (PyInt_AsLong(value)!=0) ; return 0; }
            SET(EntObs)             { self->ent->obstructedByEntities = (PyInt_AsLong(value)!=0) ; return 0; }

            SET(SpriteName) {
                engine->sprite.Free(self->ent->sprite);
                self->ent->sprite = engine->sprite.Load(PyString_AsString(value), engine->video);

                Direction dir = self->ent->direction;
                if (!self->ent->isMoving)
                    self->ent->SetAnimScript(self->ent->sprite->GetWalkScript(dir));
                else
                    self->ent->SetAnimScript(self->ent->sprite->GetIdleScript(dir));
                self->ent->UpdateAnimation();
                return 0;
            }
#undef SET
#undef GET

        PyGetSetDef properties[] = {
            {   "x",                (getter)getX,                   (setter)setX,               "Gets or sets the entity's X position. (in pixels)" },
            {   "y",                (getter)getY,                   (setter)setY,               "Gets or sets the entity's Y position. (in pixels)" },
            {   "layer",            (getter)getLayer,               (setter)setLayer,           "Gets or sets the index of the layer that the entity exists on."    },
            {   "speed",            (getter)getSpeed,               (setter)setSpeed,           "Gets or sets the entity's speed, in pixels/second" },
            {   "direction",        (getter)getDirection,           (setter)setDirection,       "Gets or sets the entity's direction"   },
            {   "curframe",         (getter)getCurFrame,            0,                          "Gets the entity's currently displayed frame"   },
            {   "specframe",        (getter)getSpecFrame,           (setter)setSpecFrame,       "If not -1, this frame is displayed instead of the normal animation" },
            {   "specanim",         (getter)getSpecAnim,            (setter)setSpecAnim,        "If not None, this animation strand is used instead of the normal animation scripts."   },
            {   "visible",          (getter)getVisible,             (setter)setVisible,         "If nonzero, the entity is drawn when onscreen" },
            {   "name",             (getter)getName,                (setter)setName,            "Gets or sets the entity's name.  This is more or less for your own convenience only."  },
            {   "movescript",       (getter)getMoveScript,          (setter)setMoveScript,      "Gets or sets the entity's current move script."    },
            {   "actscript",        (getter)getActScript,           (setter)setActScript,       "Gets or sets the object called when the entity is activated."    },
            {   "renderscript",     (getter)getRenderScript,        (setter)setRenderScript,    "Gets or sets a script to be called when the entity is drawn.\n"
                                                                                                "If None, the default behaviour is performed. (ie to draw the\n"
                                                                                                "current frame at the current position)\n"
                                                                                                "The function should be of the form myscript(entity, x, y, frame),\n"
                                                                                                "where x and y are the screen coordinates where the entity\n"
                                                                                                "would normally be drawn, and frame is the frame that would\n"
                                                                                                "be displayed." 
            },
            {   "adjacentactivate", (getter)getAdjacentActivate,    (setter)setAdjacentActivate, "Gets or sets the object called when the entity touches another entity." },
            //{   "autoface",         (getter)getAutoFace,            (setter)setAutoFace,        "If nonzero, the entity will automatically face the player when activated. (not implemented)"  },
            {   "isobs",            (getter)getIsObs,               (setter)setIsObs,           "If nonzero, the entity will obstruct other entities."  },
            {   "mapobs",           (getter)getMapObs,              (setter)setMapObs,          "If nonzero, the entity is unable to walk on obstructed areas of the map."  },
            {   "entobs",           (getter)getEntObs,              (setter)setEntObs,          "If nonzero, the entity is unable to walk through entities whose isobs property is set."    },
            {   "spritename",       (getter)getSpriteName,          (setter)setSpriteName,      "Gets or sets the filename of the sprite used to display the entity.  Setting this will load a spriteset and make the sprite use it."   },
            {   "spritewidth",      (getter)getSpriteWidth,         0,                          "Gets the width of the entity's sprite."  },
            {   "spriteheight",     (getter)getSpriteHeight,        0,                          "Gets the height of the entity's sprite."  },
            {   "hotx",             (getter)getHotX,                0,                          "Gets the X position of the entity's hotspot."  },
            {   "hoty",             (getter)getHotY,                0,                          "Gets the Y position of the entity's hotspot."  },
            {   "hotwidth",         (getter)getHotWidth,            0,                          "Gets the width of the entity's hotspot."  },
            {   "hotheight",        (getter)getHotHeight,           0,                          "Gets the height of the entity's hotspot."  },
            {   0   }
        };

        void Init() {
            memset(&type, 0, sizeof type);

            type.ob_refcnt = 1;
            type.ob_type = &PyType_Type;
            type.tp_name = "Entity";
            type.tp_basicsize = sizeof(EntityObject);
            type.tp_dealloc = (destructor)Destroy;
            type.tp_methods = methods;
            type.tp_getset = properties;
            type.tp_doc = "Entity(x, y, layer, spritename) -> Entity\n\n"
                          "Creates a new entity at pixel coordinates (x,y) on the\n"
                          "layer index specified.  The new entity uses the sprite\n"
                          "indicated.";

            type.tp_new = New;

            PyType_Ready(&type);
        }

        PyObject* New(::Entity* e) {
            EntityObject* ent = PyObject_New(EntityObject, &type);
            if (!ent) {
                return 0;
            }

            ent->ent = e;

            instances[ent->ent] = ent;

            return (PyObject*)ent;
        }

        // This is much more complicated than it should be.
        PyObject* New(PyTypeObject* /*type*/, PyObject* args, PyObject* kw) {
            static char* keywords[] = { "x", "y", "layer", "spritename", 0 };

            int x, y;
            uint layer;
            char* spritename;

            if (!PyArg_ParseTupleAndKeywords(args, kw, "iiis:Entity", keywords, &x, &y, &layer, &spritename)) {
                return 0;
            }

            if (layer >= engine->map.NumLayers()) {
                PyErr_SetString(PyExc_RuntimeError,
                    va("Map only has %i layers.  Cannot put an entity on layer %i", engine->map.NumLayers(), layer)
                );
                return 0;
            }

            Sprite* sprite;
            try {
                sprite = engine->sprite.Load(spritename, engine->video);

            } catch (std::runtime_error error) {
                PyErr_SetString(PyExc_IOError, va("sprite.Load(\"%s\") failed: %s", spritename, error.what()));
                return 0;
            }

            assert(sprite != 0);

            ::Entity* e = engine->SpawnEntity();
            e->x = x;
            e->y = y;
            e->layerIndex = layer;
            e->sprite = sprite;

            return New(e);
        }

        void Destroy(EntityObject* self) {
            assert(self->ent);

            engine->DestroyEntity(self->ent);

            instances.erase(self->ent);

            PyObject_Del(self);
        }

#define METHOD(x) PyObject* x(EntityObject* self, PyObject* args)
#define METHOD1(x) PyObject* x(EntityObject* self, PyObject* /*args*/)

        METHOD(Entity_MoveTo) {
            int x, y;

            if (!PyArg_ParseTuple(args, "ii:Entity.MoveTo", &x, &y)) {
                return 0;
            }

            self->ent->MoveTo(x, y);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Entity_Wait) {
            int time;

            if (!PyArg_ParseTuple(args, "i:Entity.Wait", &time)) {
                return 0;
            }

            self->ent->Wait(time);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Entity_Stop) {
            if (!PyArg_ParseTuple(args, "")) {
                return 0;
            }

            self->ent->Stop();

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Entity_IsMoving) {
            if (!PyArg_ParseTuple(args, "")) {
                return 0;
            }

            return PyInt_FromLong(self->ent->isMoving ? 1 : 0);
        }

        METHOD(Entity_DetectCollision) {
            if (!PyArg_ParseTuple(args, "")) {
                return 0;
            }

            const ::Entity* e1 = self->ent;

            const int x1 = e1->x;
            const int y1 = e1->y;
            const int x2 = x1 + e1->sprite->nHotw;
            const int y2 = y1 + e1->sprite->nHoth;

            foreach (const EntityPair& iter, instances) {
                EntityObject* entObj = iter.second;
                const ::Entity* e2 = iter.first;

                if ((e1 != e2)                        &&
                    (x1 <= e2->x + e2->sprite->nHotw) &&
                    (y1 <= e2->y + e2->sprite->nHoth) &&
                    (x2 >= e2->x)                     &&
                    (y2 >= e2->y))
                {
                    Py_INCREF(entObj);
                    return (PyObject*)entObj;
                }
            }

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Entity_Touches) {
            EntityObject* e2;

            if (!PyArg_ParseTuple(args, "O!", &type, &e2)) {
                return 0;
            }

            ::Entity* e = e2->ent;
            Sprite* s = e->sprite;

            int x1 = self->ent->x;
            int y1 = self->ent->y;
            int w = self->ent->sprite->nHotw;
            int h = self->ent->sprite->nHoth;

            if (x1     > e->x+s->nHotw ||
                y1     > e->y+s->nHoth ||
                x1 + w < e->x ||
                y1 + h < e->y)
            {
                Py_INCREF(Py_False);
                return Py_False;
            } else {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }

        METHOD(Entity_Render) {
            if (!PyArg_ParseTuple(args, ":Render")) {
                return 0;
            }

            engine->RenderEntity(self->ent);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Entity_Draw) {
            const int MAGIC_DEFAULT_VALUE = 0x0FFFFFFF; // biggest possible 32 bit value

            const ::Entity* ent = self->ent;

            int x = MAGIC_DEFAULT_VALUE;
            int y = MAGIC_DEFAULT_VALUE;
            int frame = ent->specFrame == -1 ? ent->curFrame : ent->specFrame;

            if (!PyArg_ParseTuple(args, "|iii:Draw", &x, &y, &frame)) {
                return 0;
            }

            if (x == MAGIC_DEFAULT_VALUE && y == MAGIC_DEFAULT_VALUE) {
                engine->DrawEntity(ent);
            } else {
                engine->DrawEntity(ent, x, y, frame);
            }

            Py_INCREF(Py_None);
            return Py_None;
        }


        METHOD1(Entity_Update) {
            self->ent->Update();

            Py_INCREF(Py_None);
            return Py_None;
        }


        METHOD(Entity_GetAnimScript) {
            char* scriptName;

            if (!PyArg_ParseTuple(args, "s:GetAnimScript", &scriptName)) {
                return 0;
            }

            return PyString_FromString(self->ent->sprite->GetScript(scriptName).c_str());
        }

        METHOD(Entity_GetAllAnimScripts) {
            if (!PyArg_ParseTuple(args, ":GetAllAnimScripts")) {
                return 0;
            }

            PyObject* dict = PyDict_New();

            const std::map<std::string, std::string>& scripts = self->ent->sprite->GetAllScripts();

            typedef std::pair<std::string, std::string> StringPair;
            foreach (const StringPair& iter, scripts) {
                PyDict_SetItemString(dict, iter.first.c_str(), PyString_FromString(iter.second.c_str()));
            }

            return dict;
        }
    }
}

#undef METHOD
#undef METHOD1
