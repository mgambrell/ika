/*
Python font object
*/

#include "ObjectDefs.h"
#include "Font.h"
#include "main.h"

namespace Script
{
    namespace Font
    {
        PyTypeObject type;

        PyMethodDef methods[] =
        {
            {   "Print",        (PyCFunction)Font_Print,        1   },
            {   "CenterPrint",  (PyCFunction)Font_CenterPrint,  1   },
            {   "RightPrint",   (PyCFunction)Font_RightPrint,   1   },
            {   "StringWidth",  (PyCFunction)Font_StringWidth,  1   },
            {   NULL,           NULL    }
        };

        void Init()
        {
            memset(&type, 0, sizeof type);

            type.ob_refcnt=1;
            type.ob_type=&PyType_Type;
            type.tp_name="Font";
            type.tp_basicsize=sizeof type;
            type.tp_dealloc=(destructor)Destroy;
            type.tp_getattr=(getattrfunc)GetAttr;
            type.tp_setattr=(setattrfunc)SetAttr;
            type.tp_doc="An ika font.\nTODO: say something interesting in here. :P";
        }

        PyObject* New(PyObject* self, PyObject* args)
        {
            char* filename;

            if (!PyArg_ParseTuple(args, "s:Font", &filename))
                return NULL;

            FontObject* font=PyObject_New(FontObject, &type);
            if (!font)
                return NULL;

            try
            {
                font->font=new CFont(filename, engine->video);
            }
            catch (FontException)
            {
                PyErr_SetString(PyExc_OSError, va("Failed to load %s", filename));
                return NULL;
            }

            return (PyObject*)font;
        }

        void Destroy(FontObject* self)
        {
            delete self->font;

            PyObject_Del(self);
        }

        PyObject* GetAttr(FontObject* self, char* name)
        {
            if (!strcmp(name, "width"))
                return PyInt_FromLong(self->font->Width());
            if (!strcmp(name, "height"))
                return PyInt_FromLong(self->font->Height());
            if (!strcmp(name, "tabsize"))
                return PyInt_FromLong(self->font->TabSize());

            return Py_FindMethod(methods, (PyObject*)self, name);
        }

        int SetAttr(FontObject* self, char* name, PyObject* value)
        {
            if (!strcmp(name, "tabsize"))
                self->font->SetTabSize(PyInt_AsLong(value));

            return 0;
        }

#define METHOD(x) PyObject* x(FontObject* self, PyObject* args)

        METHOD(Font_Print)
        {
            int x, y;
            char* message;

            if (!PyArg_ParseTuple(args, "iis:Font.Print", &x, &y, &message))
                return NULL;

            self->font->PrintString(x, y, message);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Font_CenterPrint)
        {
            int x, y;
            char* message;

            if (!PyArg_ParseTuple(args, "iis:Font.CenterPrint", &x, &y, &message))
                return NULL;

            CFont* f=self->font;
            f->PrintString(x- f->StringWidth(message)/2 , y, message);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Font_RightPrint)
        {
            int x, y;
            char* message;

            if (!PyArg_ParseTuple(args, "iis:Font.RightPrint", &x, &y, &message))
                return NULL;

            CFont* f=self->font;
            f->PrintString(x- f->StringWidth(message) , y, message);

            Py_INCREF(Py_None);
            return Py_None;
        }

        METHOD(Font_StringWidth)
        {
            char* message;

            if (!PyArg_ParseTuple(args, "s:Font.Width", &message))
                return NULL;

            return PyInt_FromLong(self->font->StringWidth(message));
        }

#undef METHOD
    }
}