/*
Python interface for input.
*/

#include "common/misc.h"
#include "ObjectDefs.h"
#include "input.h"
#include "main.h"

namespace Script
{
    namespace Input
    {
        PyTypeObject type;
        PyMappingMethods mappingmethods;

        PyMethodDef methods[] =
        {
            {   "Update",           (PyCFunction)Input_Update,      METH_NOARGS,
                "Input.Update()\n\n"
                "Updates the state of the mouse, and any attached input devices.\n"
                "Also gives the OS a chance to do background tasks.  Continious\n"
                "loops should call this occasionally to give the OS time to perform\n"
                "its tasks."
            },
            
            {   "Unpress",          (PyCFunction)Input_Unpress,     METH_NOARGS,
                "Input.Unpress()\n\n"
                "Unsets the Pressed() property of all controls.  Has no effect on\n"
                "their positions.\n"
                "Individual controls can be unpressed by simply calling their Pressed()\n"
                "method, discarding the result."
            },

            {   "GetControl",       (PyCFunction)Input_GetControl,  METH_VARARGS,
                "Input.GetControl(name -> string)\n\n"
                "Returns an object that represents the requested control.\n"
                "Input[name->string] performs the exact same function.\n"
                "ie x = ika.Input['UP'] or x = ika.Input.GetControl('UP')"
            },

            {   "GetKey",           (PyCFunction)Input_GetKey,  METH_NOARGS,
                "Input.GetKey() -> char or None\n\n"
                "Returns the next key in the keyboard queue, or None if the queue is empty."
            },

            {   "ClearKeyQueue",    (PyCFunction)Input_ClearKeyQueue,   METH_NOARGS,
                "Input.ClearKeyQueue()\n\n"
                "Flushes the keyboard queue.  Any keypresses that were in the queue will not\n"
                "be returned by Input.GetKey()"
            },

            {   "WasKeyPressed",    (PyCFunction)Input_WasKeyPressed,   METH_NOARGS,
                "Input.WasKeyPressed() -> bool\n\n"
                "Returns True if there is a key in the key queue.  False if not.\n"
                "(ie Input.GetKey() will return None if the result is False)"
            },

            {   0   }
        };

        // proto
        PyObject* Input_Subscript(InputObject* self, PyObject* key);

#define GET(x) PyObject* get ## x(InputObject* self)
        GET(Up)     { return Input_GetControl(self, Py_BuildValue("(s)", "UP"));     }
        GET(Down)   { return Input_GetControl(self, Py_BuildValue("(s)", "DOWN"));   }
        GET(Left)   { return Input_GetControl(self, Py_BuildValue("(s)", "LEFT"));   }
        GET(Right)  { return Input_GetControl(self, Py_BuildValue("(s)", "RIGHT"));  }
        GET(Enter)  { return Input_GetControl(self, Py_BuildValue("(s)", "RETURN")); }
        GET(Cancel) { return Input_GetControl(self, Py_BuildValue("(s)", "ESCAPE")); }
#undef GET

        PyGetSetDef properties[] =
        {
            {   "up",       (getter)getUp,      0,  "Gets the standard \"Up\" control."     },
            {   "down",     (getter)getDown,    0,  "Gets the standard \"Down\" control."   },
            {   "left",     (getter)getLeft,    0,  "Gets the standard \"Left\" control."   },
            {   "right",    (getter)getRight,   0,  "Gets the standard \"Right\" control."  },
            {   "enter",    (getter)getEnter,   0,  "Gets the standard \"Enter\" control."  },
            {   "cancel",   (getter)getCancel,  0,  "Gets the standard \"Cancel\" control." },
            {   0   }
        };

        void Init()
        {
            memset(&type, 0, sizeof type);

            mappingmethods.mp_length = 0;
            mappingmethods.mp_subscript = (binaryfunc)&Input_Subscript;
            mappingmethods.mp_ass_subscript = 0;

            type.ob_refcnt = 1;
            type.ob_type = &PyType_Type;
            type.tp_name = "Input";
            type.tp_basicsize = sizeof type;
            type.tp_dealloc = (destructor)Destroy;
            type.tp_methods = methods;
            type.tp_getset = properties;
            type.tp_as_mapping = &mappingmethods;
            type.tp_doc = "Interface for hardware input devices. (such as the keyboard and mouse)";

            PyType_Ready(&type);
        }

        PyObject* New(::Input& i)
        {
            InputObject* input=PyObject_New(InputObject, &type);

            if (!input)
                return NULL;

            input->input = &i;

            return (PyObject*)input;
        }

        void Destroy(InputObject* self)
        {
            PyObject_Del(self);
        }

#define METHOD(x)  PyObject* x(InputObject* self, PyObject* args)
#define METHOD1(x) PyObject* x(InputObject* self)

        METHOD1(Input_Update)
        {
            engine->CheckMessages();

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD1(Input_Unpress)
        {
            self->input->Unpress();

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Input_GetControl)
        {
            PyObject* obj;
            if (!PyArg_ParseTuple(args, "O:Input.GetControl", &obj))
                return 0;

            return Input_Subscript(self, obj);
        }

        METHOD1(Input_GetKey)
        {
            char c = self->input->GetKey();

            if (c)
                return PyString_FromStringAndSize(&c, 1);   // wtf.  No PyString_FromChar
            else
            {
                Py_INCREF(Py_None);
                return Py_None;
            }
        }

        METHOD1(Input_ClearKeyQueue)
        {
            self->input->ClearKeyQueue();
            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD1(Input_WasKeyPressed)
        {
            return PyInt_FromLong(self->input->WasKeyPressed() ? 1 : 0);
        }

#undef METHOD
#undef METHOD1

        PyObject* Input_Subscript(InputObject* self, PyObject* key)
        {
            try
            {
                const char* name = PyString_AsString(key);
                if (!name)
                    throw "Non-string passed as input control name.";

                PyObject* obj = Script::Control::New(*self->input, name);

                if (!obj)
                    throw va("%s is not a valid input control name.", name);

                return obj;
            }
            catch (const char* s)
            {
                PyErr_SetString(PyExc_SyntaxError, s);
                return 0;
            }
        }
    }
}
