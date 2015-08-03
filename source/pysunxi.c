/*
 * SUNXI_GPIO.c
 *
 * Copyright 2013 Stefan Mavrodiev <support@olimex.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#include "Python.h"
#include "gpio_lib.h"


#define PD_COUNT 28
#define PG_COUNT 12

#define MISO    SUNXI_GPE(3)
#define MOSI    SUNXI_GPE(2)
#define SCK     SUNXI_GPE(1)
#define CS      SUNXI_GPE(0)


static PyObject *SetupException;
static PyObject *OutputException;
static PyObject *InputException;
static PyObject *inp;
static PyObject *out;
static PyObject *per;
static PyObject *high;
static PyObject *low;


static int module_setup(void)
{
    int result;

    result = sunxi_gpio_init();
    
    if (result == SETUP_DEVMEM_FAIL)
    {
        PyErr_SetString(SetupException, "No access to /dev/mem. Try running as root!");
        return SETUP_DEVMEM_FAIL;
    }
    else if (result == SETUP_MALLOC_FAIL)
    {
        PyErr_NoMemory();
        return SETUP_MALLOC_FAIL;
    }
    else if (result == SETUP_MMAP_FAIL)
    {
        PyErr_SetString(SetupException, "Mmap failed on module import");
        return SETUP_MMAP_FAIL;
    }
    else
    {
        return SETUP_OK;
    }
}



static PyObject*
py_output(PyObject* self, PyObject* args)
{
    int gpio;
    int value;

    if(!PyArg_ParseTuple(args, "ii", &gpio, &value))
        return NULL;

    if(value != 0 && value != 1)
    {
        PyErr_SetString(OutputException, "Invalid output state");
        return NULL;
    }

    if(sunxi_gpio_get_cfgpin(gpio) != SUNXI_GPIO_OUTPUT)
    {
        PyErr_SetString(OutputException, "GPIO is no an output");
        return NULL;
    }
    sunxi_gpio_output(gpio, value);

    Py_RETURN_NONE;
}

static PyObject*
py_input(PyObject* self, PyObject* args)
{
    int gpio;
    int result;

    if(!PyArg_ParseTuple(args, "i", &gpio))
        return NULL;

    if(sunxi_gpio_get_cfgpin(gpio) != SUNXI_GPIO_INPUT)
    {
        PyErr_SetString(InputException, "GPIO is not an input");
        return NULL;
    }
    result = sunxi_gpio_input(gpio);

    if(result == -1)
    {
        PyErr_SetString(InputException, "Reading pin failed");
        return NULL;
    }

    return Py_BuildValue("i", result);
}

static PyObject*
py_setcfg(PyObject* self, PyObject* args)
{
    int gpio;
    int direction;

    if(!PyArg_ParseTuple(args, "ii", &gpio, &direction))
        return NULL;

    if(direction != INPUT && direction != OUTPUT && direction != PER)
    {
        PyErr_SetString(SetupException, "Invalid direction");
        return NULL;
    }
    sunxi_gpio_set_cfgpin(gpio, direction);

    Py_RETURN_NONE;
}

static PyObject*
py_getcfg(PyObject* self, PyObject* args)
{
    int gpio;
    int result;

    if(!PyArg_ParseTuple(args, "i", &gpio))
        return NULL;

    result = sunxi_gpio_get_cfgpin(gpio);

    return Py_BuildValue("i", result);
}

static PyObject*
py_init(PyObject* self, PyObject* args)
{
    module_setup();
    Py_RETURN_NONE;
}

static PyObject*
py_cleanup(PyObject* self, PyObject* args)
{
    sunxi_gpio_cleanup();
    Py_RETURN_NONE;
}


PyMethodDef module_methods[] =
{
    {"init", py_init, METH_NOARGS, "Initialize module"},
    {"cleanup", py_cleanup, METH_NOARGS, "munmap /dev/map."},
    {"setcfg", py_setcfg, METH_VARARGS, "Set direction."},
    {"getcfg", py_getcfg, METH_VARARGS, "Get direction."},
    {"output", py_output, METH_VARARGS, "Set output state"},
    {"input", py_input, METH_VARARGS, "Get input state"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef module_def =
{
    PyModuleDef_HEAD_INIT,
    "SUNXI_GPIO",
    NULL,
    -1,
    module_methods
};
#endif

PyMODINIT_FUNC PyInit_SUNXI_GPIO(void)
{
    int i;
    char buf[10];
    PyObject *module = NULL;

#if PY_MAJOR_VERSION >= 3
    module = PyModule_Create(&module_def);
#else
    module = Py_InitModule("SUNXI_GPIO", module_methods);
#endif

    if (module == NULL)
#if PY_MAJOR_VERSION >= 3
        return module;
#else
        return;
#endif
    
    SetupException = PyErr_NewException("SUNXI_GPIO.SetupException", NULL, NULL);
    Py_INCREF(SetupException);
    PyModule_AddObject(module, "SetupException", SetupException);

    OutputException = PyErr_NewException("SUNXI_GPIO.OutputException", NULL, NULL);
    Py_INCREF(OutputException);
    PyModule_AddObject(module, "OutputException", OutputException);

    InputException = PyErr_NewException("SUNXI_GPIO.InputException", NULL, NULL);
    Py_INCREF(InputException);
    PyModule_AddObject(module, "InputException", InputException);

    
    high = Py_BuildValue("i", HIGH);
    low = Py_BuildValue("i", LOW);
    inp = Py_BuildValue("i", INPUT);
    out = Py_BuildValue("i", OUTPUT);
    per = Py_BuildValue("i", PER);
    PyModule_AddObject(module, "HIGH", high);
    PyModule_AddObject(module, "LOW", low);
    PyModule_AddObject(module, "IN", inp);
    PyModule_AddObject(module, "OUT", out);
    PyModule_AddObject(module, "PER", per);


    for (i = 0; i < PD_COUNT; ++i)
    {
        sprintf(buf, "PD%d", i);
        PyModule_AddObject(module, buf, Py_BuildValue("i", SUNXI_GPD(i)));
    }

    for (i = 0; i < PG_COUNT; ++i)
    {
        sprintf(buf, "PG%d", i);
        PyModule_AddObject(module, buf, Py_BuildValue("i", SUNXI_GPG(i)));
    }
        
    PyModule_AddObject(module, "MISO", Py_BuildValue("i", MISO));
    PyModule_AddObject(module, "MOSI", Py_BuildValue("i", MOSI));
    PyModule_AddObject(module, "SCK", Py_BuildValue("i", SCK));
    PyModule_AddObject(module, "CS", Py_BuildValue("i", CS));

    if (Py_AtExit(sunxi_gpio_cleanup) != 0)
    {
        sunxi_gpio_cleanup();

#if PY_MAJOR_VERSION >= 3
        return NULL;
#else
        return;
#endif
    }
    return module;
}
