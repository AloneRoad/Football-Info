#include "Python.h"
#include <string.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


static PyObject *UDNResolveError;
static PyObject *UnresolvedEntity;
static PyObject *PlaceholderError; 
static PyObject *UnresolvedPlaceholder;

#define TRUE 1
#define FALSE 0


static void
set_udn_resolve_error(char *name, PyObject *namespace)
{
  PyObject *exception_args = NULL;

  exception_args = Py_BuildValue("s", name);
  /* exception_args = Py_BuildValue("sO", name, namespace); */
  PyErr_SetObject(UDNResolveError, exception_args);
  Py_DECREF(exception_args);
}


static PyObject *
_resolve_udn(PyObject *obj, char *name, int raise_exception)
{
  PyObject *return_value = NULL;

  if (PyObject_HasAttrString(obj, name)) {
    return_value = PyObject_GetAttrString(obj, name);
  } else if (PyMapping_Check(obj) && PyMapping_HasKeyString(obj, name)) {
    return_value = PyMapping_GetItemString(obj, name);
  } else {
    if (raise_exception) {
      set_udn_resolve_error(name, obj);
    } else {
      /* other functions here return a new reference, so keep it consistent and
         just increment the ref on UnresolvedEntity.
      */
      Py_INCREF(UnresolvedEntity);
      return_value = UnresolvedEntity;
    }
  }

  return return_value;
}


/* Wrapper functions to export into the Python module */

static PyObject *
udn_resolve_udn(PyObject *self, PyObject *args, PyObject *kargs)
{
  PyObject *obj;
  char *name;
  int raise_exception = 0;

  static char *karg_list[] = {"obj", "name", "raise_exception", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kargs, "Os|i", karg_list,
                                   &obj, &name, &raise_exception)) {
    return NULL;
  }

  return _resolve_udn(obj, name, raise_exception);
}


static PyObject *
udn_resolve_from_search_list(PyObject *self, PyObject *args, PyObject *keywds)
{
  PyObject *name_space = NULL;
  PyObject *return_value = NULL;
  PyObject *iterator = NULL;

  PyObject *search_list = NULL;
  char *name;
  PyObject *default_value = NULL;

  static char *kwlist[] = {"search_list", "name", "default", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "Os|O", kwlist,
                                   &search_list, &name, &default_value)) {
    return NULL;
  }

  iterator = PyObject_GetIter(search_list);
  if (iterator == NULL) {
    return_value = UnresolvedEntity;
    Py_INCREF(return_value);
    /* PyErr_SetString(PyExc_TypeError, "search_list is not iterable"); */
    goto done;
  }

  while ((name_space = PyIter_Next(iterator))) {
    return_value = _resolve_udn(name_space, name, 0);
    Py_DECREF(name_space);
    if (return_value != UnresolvedEntity) {
      goto done;
    }
  }
  if (return_value == UnresolvedEntity) {
    if (default_value != NULL) {
      return_value = default_value;
      Py_INCREF(return_value);
    } else {
      Py_DECREF(return_value);
      return_value = UnresolvedPlaceholder;
      Py_INCREF(return_value);
    }
    goto done;
  }
 done:
  Py_XDECREF(iterator);
  /* change the return value to be a bit more compatible with the way things
     work in the python code.
   */ 
  return return_value;
}


/* Method registration table: name-string -> function-pointer */

static struct PyMethodDef udn_methods[] = {
  {"_resolve_udn", (PyCFunction)udn_resolve_udn, METH_VARARGS|METH_KEYWORDS},
  {"_resolve_from_search_list", (PyCFunction)udn_resolve_from_search_list, METH_VARARGS|METH_KEYWORDS},
  {NULL, NULL}
};


/* Initialization function (import-time) */

DL_EXPORT(void)
init_udn(void)
{
  PyObject *m, *runtime_module;

  m = Py_InitModule("_udn", udn_methods);
  
  runtime_module = PyImport_ImportModule("spitfire.runtime");
  PlaceholderError = PyObject_GetAttrString(
    runtime_module, "PlaceholderError");
  UDNResolveError = PyObject_GetAttrString(runtime_module, "UDNResolveError");
  UnresolvedPlaceholder = PyObject_GetAttrString(
    runtime_module, "UnresolvedPlaceholder");
  UnresolvedEntity = PyObject_GetAttrString(
    runtime_module, "UnresolvedEntity");
  Py_DECREF(runtime_module);

  if (PyErr_Occurred())
    Py_FatalError("Can't initialize module _udn");
}

#ifdef __cplusplus
}
#endif
