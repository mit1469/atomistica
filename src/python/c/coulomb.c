/* ======================================================================
   Atomistica - Interatomic potential library
   https://github.com/pastewka/atomistica
   Lars Pastewka, lars.pastewka@iwm.fraunhofer.de, and others
   See the AUTHORS file in the top-level Atomistica directory.

   Copyright (2005-2013) Fraunhofer IWM

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 2 of the License, or
   (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
  
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ====================================================================== */

#include <Python.h>
#define PY_ARRAY_UNIQUE_SYMBOL ATOMISTICA_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

#include "error.h"

#include "atomisticamodule.h"

#include "coulomb.h"

#include "py_f.h"
#include "particles.h"
#include "neighbors.h"


/* Python object types:
   Coulomb - Single coulomb
*/

/* Coulomb methods and class
 */


static PyObject *
coulomb_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  coulomb_t *self;
  section_t *zero;
  int i;
  char *name;
  char errstr[200];

#ifdef DEBUG
  printf("[coulomb_new] %p %p %p\n", type, args, kwds);
#endif

  self = (coulomb_t *)type->tp_alloc(type, 0);
  if (self != NULL) {

    /* FIXME: the offset *12* assumes the namespace is always _atomistica.* */
    name  = strdup(self->ob_type->tp_name + 12);

#ifdef DEBUG
    printf("[coulomb_new] Coulomb name: %s\n", name);
#endif

    self->f90class  = NULL;
    for (i = 0; i < N_COULOMB_CLASSES; i++) {
      if (!strcmp(name, coulomb_classes[i].name))
        self->f90class  = &coulomb_classes[i];
    }
      
    if (!self->f90class) {
      sprintf(errstr, "Internal error: Coulomb not found: %s\n", name);
      PyErr_SetString(PyExc_TypeError, errstr);
      return NULL;
    }

    zero = NULL;
    self->f90class->new_instance(&self->f90obj, zero, &self->f90members);
#ifdef DEBUG
    printf("[coulomb_new] pointer = %p\n", self->f90obj);
#endif
  }

  return (PyObject *) self;
}


static void
coulomb_dealloc(coulomb_t *self)
{
#ifdef DEBUG
  printf("[coulomb_dealloc] %p\n", self);
#endif

  self->f90class->free_instance(self->f90obj);

  self->ob_type->tp_free((PyObject*) self);
}


static int
coulomb_init(coulomb_t *self, PyObject *args, PyObject *kwargs)
{
  int ierror = ERROR_NONE;

#ifdef DEBUG
  printf("[coulomb_init] %p %p %p\n", self, args, kwargs);
#endif

  if (kwargs) {
    if (pydict_to_ptrdict(kwargs, self->f90members))
      return -1;
  }

  self->f90class->init(self->f90obj, &ierror);

  if (error_to_py(ierror))
    return -1;

#ifdef DEBUG
  printf("{coulomb_init}\n");
#endif

  return 0;
}


/* Attribute set/getters */

static PyObject *
coulomb_getattro(coulomb_t *self, PyObject *pyname)
{
  char *name;
  property_t *p;
  PyObject *r = NULL;
  PyArrayObject *arr;
  npy_intp dims[3];
  double *data;
  PyObject **odata;
  int i, j, k;

  name = PyString_AsString(pyname);
  if (!name)
    return NULL;

  p = self->f90members->first_property;

  while (p != NULL && strcmp(p->name, name)) {
    p = p->next;
  }

  if (p) {
    r = NULL;
    switch (p->kind) {
    case PK_INT:
      r = PyInt_FromLong(*((int*) p->ptr));
      break;
    case PK_DOUBLE:
      r = PyFloat_FromDouble(*((double*) p->ptr));
      break;
    case PK_BOOL:
      r = PyBool_FromLong(*((BOOL*) p->ptr));
      break;
    case PK_LIST:
      if (*p->tag5 == 1) {
	r = PyFloat_FromDouble(*((double*) p->ptr));
      } else {
	dims[0] = *p->tag5;
	arr = (PyArrayObject*) PyArray_SimpleNew(1, (npy_intp*) dims,
						 NPY_DOUBLE);
	data = (double *) PyArray_DATA(arr);
	for (i = 0; i < *p->tag5; i++) {
	  data[i] = ((double*) p->ptr)[i];
	}
	r = (PyObject*) arr;
      }
      break;
    case PK_FORTRAN_STRING_LIST:
      if (*p->tag5 == 1) {
	r = fstring_to_pystring((char*) p->ptr, p->tag);
      } else {
	dims[0] = *p->tag5;
	arr = (PyArrayObject*) PyArray_SimpleNew(1, (npy_intp*) dims,
						 NPY_OBJECT);
	odata = (PyObject **) PyArray_DATA(arr);
	for (i = 0; i < *p->tag5; i++) {
	  odata[i] = fstring_to_pystring(((char*) p->ptr + i*p->tag), p->tag);
	}
	r = (PyObject*) arr;
      }
      break;
    case PK_ARRAY2D:
      dims[0] = p->tag;
      dims[1] = p->tag2;
      arr = (PyArrayObject*) PyArray_SimpleNew(2, (npy_intp*) dims, NPY_DOUBLE);
      data = (double *) PyArray_DATA(arr);
      for (i = 0; i < p->tag; i++) {
	for (j = 0; j < p->tag2; j++) {
	  data[j + i*p->tag2] = ((double*) p->ptr)[i + j*p->tag];
	}
      }
      //        memcpy(data, p->ptr, p->tag*p->tag2*sizeof(double));
      r = (PyObject*) arr;
      break;
    case PK_ARRAY3D:
      dims[0] = p->tag;
      dims[1] = p->tag2;
      dims[2] = p->tag3;
      arr = (PyArrayObject*) PyArray_SimpleNew(3, (npy_intp*) dims, NPY_DOUBLE);
      data = (double *) PyArray_DATA(arr);
      for (i = 0; i < p->tag; i++) {
	for (j = 0; j < p->tag2; j++) {
	  for (k = 0; k < p->tag3; k++) {
	    data[k + (j + i*p->tag2)*p->tag3] = 
	      ((double*) p->ptr)[i + (j + k*p->tag2)*p->tag];
	  }
	}
      }
      //        memcpy(data, p->ptr, p->tag*p->tag2*p->tag3*sizeof(double));
      r = (PyObject*) arr;
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "Internal error: Unknown type encountered in section.");
      break;
    }
  } else {
    r = PyObject_GenericGetAttr((PyObject *) self, pyname);
    // Py_FindMethod(self->ob_type->tp_methods, (PyObject *) self, name);
  }

  return r;
}


/* Methods */

static PyObject *
coulomb_str(coulomb_t *self, PyObject *args)
{
  return PyString_FromString(self->f90class->name);
}


static PyObject *
coulomb_register_data(coulomb_t *self, PyObject *args)
{
  particles_t *a;
  int ierror = ERROR_NONE;

  if (!PyArg_ParseTuple(args, "O!", &particles_type, &a))
    return NULL; 

  self->f90class->register_data(self->f90obj, a->f90obj, &ierror);

  if (error_to_py(ierror))
    return NULL;

  Py_RETURN_NONE;
}


static PyObject *
coulomb_set_Hubbard_U(coulomb_t *self, PyObject *args)
{
  particles_t *a;
  PyObject *U, *arr_U;
  int ierror = ERROR_NONE;

#ifdef DEBUG
  printf("[coulomb_set_Hubbard_U] self = %p\n", self);
#endif

  if (!PyArg_ParseTuple(args, "O!O", &particles_type, &a, &U))
    return NULL; 

  arr_U = PyArray_FROMANY(U, NPY_DOUBLE, 1, 1, 0);
  if (!arr_U)
    return NULL;

  if (self->f90class->set_Hubbard_U) {
    self->f90class->set_Hubbard_U(self->f90obj, a->f90obj, PyArray_DATA(arr_U),
				  &ierror);

    if (error_to_py(ierror)) {
      Py_DECREF(arr_U);
      return NULL;
    }
  }

  Py_DECREF(arr_U);

  Py_RETURN_NONE;
}


static PyObject *
coulomb_bind_to(coulomb_t *self, PyObject *args)
{
  particles_t *a;
  neighbors_t *n;
  int ierror = ERROR_NONE;

#ifdef DEBUG
  printf("[coulomb_bind_to] self = %p\n", self);
#endif

  if (!PyArg_ParseTuple(args, "O!O!", &particles_type, &a, &neighbors_type, &n))
    return NULL; 

  self->f90class->bind_to(self->f90obj, a->f90obj, n->f90obj, &ierror);

  if (error_to_py(ierror))
    return NULL;

  Py_RETURN_NONE;
}


static PyObject *
coulomb_potential_and_field(coulomb_t *self, PyObject *args, PyObject *kwargs)
{
  npy_intp dims[3];
  npy_intp strides[3];

  particles_t *a;
  neighbors_t *n;

  int ierror = ERROR_NONE;

  double epot;
  PyObject *q_in, *q;
  PyObject *phi = NULL;
  PyObject *E = NULL;
  PyObject *wpot;

  PyObject *r;

  int i;

  /* --- */

#ifdef DEBUG
  printf("[coulomb_potential_and_field] self = %p\n", self);
#endif

  if (!PyArg_ParseTuple(args, "O!O!O|O!O!", &particles_type, &a,
			&neighbors_type, &n, &q_in, &PyArray_Type, &phi,
			&PyArray_Type, &E))
    return NULL;
  
  q = PyArray_FROMANY(q_in, NPY_DOUBLE, 1, 1, 0);
  if (!q)
    return NULL;

  epot = 0.0;

  if (phi) {
    Py_INCREF(phi);
  }
  else {
    dims[0] = data_get_len(a->f90data);
    phi = PyArray_ZEROS(1, dims, NPY_DOUBLE, 1);
  }

  if (E) {
    Py_INCREF(E);
  }
  else {
    dims[0] = data_get_len(a->f90data);
    dims[1] = 3;
    strides[0] = dims[1]*NPY_SIZEOF_DOUBLE;
    strides[1] = NPY_SIZEOF_DOUBLE;
    E = (PyObject*) PyArray_New(&PyArray_Type, 2, dims, NPY_DOUBLE, strides,
				NULL, 0, NPY_FARRAY, NULL);
    memset(PyArray_DATA(E), 0, dims[0]*dims[1]*NPY_SIZEOF_DOUBLE);
  }

  dims[0] = 3;
  dims[1] = 3;
  wpot = PyArray_ZEROS(2, dims, NPY_DOUBLE, 1);

#ifdef DEBUG
  printf("[coulomb_potential_and_field] self->f90class->name = %s\n",
	 self->f90class->name);
  printf("[coulomb_potential_and_field] self->f90obj = %p\n",
	 self->f90obj);
  printf("[coulomb_potential_and_field] a->f90obj = %p\n",
	 a->f90obj);
  printf("[coulomb_potential_and_field] n->f90obj = %p\n",
	 n->f90obj);
  printf("[coulomb_potential_and_field] self->f90class->potential_and_field = %p\n",
	 self->f90class->potential_and_field);
#endif

  self->f90class->potential_and_field(self->f90obj, a->f90obj, n->f90obj,
				      PyArray_DATA(q), PyArray_DATA(phi), &epot,
				      PyArray_DATA(E), PyArray_DATA(wpot),
				      &ierror);

#ifdef DEBUG
  printf("[coulomb_potential_and_field] epot = %f\n", epot);
#endif

  if (error_to_py(ierror))
    return NULL;

  /* --- Compose return tuple --- */

  r  = PyTuple_New(4);
  if (!r)
    return NULL;

  PyTuple_SET_ITEM(r, 0, phi);
  PyTuple_SET_ITEM(r, 1, PyFloat_FromDouble(epot));
  PyTuple_SET_ITEM(r, 2, E);
  PyTuple_SET_ITEM(r, 3, wpot);

#ifdef DEBUG
  printf("{coulomb_potential_and_field}\n");
#endif

  Py_DECREF(q);

  return r;
}


static PyObject *
coulomb_potential(coulomb_t *self, PyObject *args, PyObject *kwargs)
{
  npy_intp dims[3];
  npy_intp strides[3];

  particles_t *a;
  neighbors_t *n;

  int ierror = ERROR_NONE;

  PyObject *q_in, *q;
  PyObject *phi = NULL;

  PyObject *r;

  int i;

  /* --- */

#ifdef DEBUG
  printf("[coulomb_potential] self = %p\n", self);
#endif

  if (!PyArg_ParseTuple(args, "O!O!O|O!", &particles_type, &a,
			&neighbors_type, &n, &q_in, &PyArray_Type, &phi))
    return NULL;
  
  q = PyArray_FROMANY(q_in, NPY_DOUBLE, 1, 1, 0);
  if (!q)
    return NULL;

  if (phi) {
    Py_INCREF(phi);
  }
  else {
    dims[0] = data_get_len(a->f90data);
    phi = PyArray_ZEROS(1, dims, NPY_DOUBLE, 1);
  }

#ifdef DEBUG
  printf("[coulomb_potential] self->f90class->name = %s\n",
	 self->f90class->name);
  printf("[coulomb_potential] self->f90obj = %p\n",
	 self->f90obj);
  printf("[coulomb_potential] a->f90obj = %p\n",
	 a->f90obj);
  printf("[coulomb_potential] n->f90obj = %p\n",
	 n->f90obj);
  printf("[coulomb_potential] self->f90class->potential = %p\n",
	 self->f90class->potential);
#endif

  self->f90class->potential(self->f90obj, a->f90obj, n->f90obj,
			    PyArray_DATA(q), PyArray_DATA(phi), &ierror);

  Py_DECREF(q);
  if (error_to_py(ierror)) {
    Py_DECREF(phi);
    return NULL;
  }

  /* --- Compose return tuple --- */

#ifdef DEBUG
  printf("{coulomb_potential}\n");
#endif

  return phi;
}


/* Methods declaration */

static PyMethodDef coulomb_methods[] = {
  { "register_data", (PyCFunction) coulomb_register_data, METH_VARARGS,
    "Register internal data fields with a particles object." },
  { "set_Hubbard_U", (PyCFunction) coulomb_set_Hubbard_U, METH_VARARGS,
    "Set the Hubbard-U value for each element." },
  { "bind_to", (PyCFunction) coulomb_bind_to, METH_VARARGS,
    "Bind this coulomb to a certain Particles and Neighbors object. This is to "
    "be called if either one changes." },
  { "potential_and_field", (PyCFunction) coulomb_potential_and_field,
    METH_VARARGS, "Compute the electrostatic potential and field." },
  { "potential", (PyCFunction) coulomb_potential,
    METH_VARARGS, "Compute the electrostatic potential." },
  { NULL, NULL, 0, NULL }  /* Sentinel */
};


PyTypeObject coulomb_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                          /*ob_size*/
    "_atomistica.Coulomb",                      /*tp_name*/
    sizeof(coulomb_t),                          /*tp_basicsize*/
    0,                                          /*tp_itemsize*/
    (destructor)coulomb_dealloc,                /*tp_dealloc*/
    0,                                          /*tp_print*/
    0,                                          /*tp_getattr*/
    0,                                          /*tp_setattr*/
    0,                                          /*tp_compare*/
    0,                                          /*tp_repr*/
    0,                                          /*tp_as_number*/
    0,                                          /*tp_as_sequence*/
    0,                                          /*tp_as_mapping*/
    0,                                          /*tp_hash */
    0,                                          /*tp_call*/
    (reprfunc)coulomb_str,                      /*tp_str*/
    (getattrofunc)coulomb_getattro,             /*tp_getattro*/
    0,                                          /*tp_setattro*/
    //    (setattrofunc)coulomb_setattro,       /*tp_setattro*/
    0,                                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
    "Coulomb objects",                          /* tp_doc */
    0,		                                /* tp_traverse */
    0,		                                /* tp_clear */
    0,		                                /* tp_richcompare */
    0,		                                /* tp_weaklistoffset */
    0,		                                /* tp_iter */
    0,		                                /* tp_iternext */
    coulomb_methods,                            /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)coulomb_init,                     /* tp_init */
    0,                                          /* tp_alloc */
    coulomb_new,                                /* tp_new */
};