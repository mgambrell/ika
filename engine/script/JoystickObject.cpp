
#include <cassert>
#include "ObjectDefs.h"
#include "input.h"
#include "joystick.h"

namespace Script
{
    namespace Joystick
    {
        PyTypeObject type;

#define GET(x) PyObject* get ## x(JoystickObject* self)
        GET(Axes)           { Py_INCREF(self->axes);  return self->axes;                }
        GET(ReverseAxes)    { Py_INCREF(self->reverseAxes); return self->reverseAxes;   }
        GET(Buttons)        { Py_INCREF(self->buttons); return self->buttons;           }
#undef GET

        PyGetSetDef properties[] =
        {
            {   "axes", (getter)getAxes,        0,      "Gets a tuple containing all of the joystick axes."     },
            {   "reverseAxes",  (getter)getReverseAxes, 0, "Gets a tuple containing all of the joystick axes.\n"
                                                        "Unlike axes, the controls in this tuple *reverse*\n"
                                                        "the direction of the axis."                            },
            {   "buttons",  (getter)getButtons, 0,      "Gets a tuple containing all of the joystick buttons."  },
            {   0   }
        };

        void Init()
        {
            memset(&type, 0, sizeof type);
            type.ob_refcnt = 1;
            type.ob_type = &PyType_Type;
            type.tp_name = "Joystick";
            type.tp_basicsize = sizeof(JoystickObject);
            type.tp_base = &Script::InputDevice::type;
            type.tp_dealloc = (destructor)Destroy;
            type.tp_getset = properties;
            type.tp_doc = "A joystick.";
            PyType_Ready(&type);
        }

        PyObject* New(::Joystick* stick)
        {
            JoystickObject* joy = PyObject_New(JoystickObject, &type);

            joy->joystick = stick;
            joy->axes = PyTuple_New(stick->GetNumAxes());
            joy->reverseAxes = PyTuple_New(stick->GetNumAxes());
            joy->buttons = PyTuple_New(stick->GetNumButtons());

            for (uint i = 0; i < stick->GetNumAxes(); i++)
            {
                PyObject* c = Script::Control::New(stick->GetAxis(i));
                PyObject* d = Script::Control::New(stick->GetReverseAxis(i));
                PyTuple_SET_ITEM(joy->axes, i, c);
                PyTuple_SET_ITEM(joy->reverseAxes, i, d);
            }

            for (uint i = 0; i < stick->GetNumButtons(); i++)
            {
                PyObject* c = Script::Control::New(stick->GetButton(i));
                PyTuple_SET_ITEM(joy->buttons, i, c);
            }

            return (PyObject*)joy;
        }

        void Destroy(JoystickObject* self)
        {
            Py_DECREF(self->axes);
            Py_DECREF(self->reverseAxes);
            Py_DECREF(self->buttons);
            PyObject_Del(self);
        }
    }
}