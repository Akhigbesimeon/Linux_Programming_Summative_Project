/* Documentation
----------------
1. Conversion Strategy:
We extract Python objects using PyList_GetItem and PyTuple_GetItem.
These return "borrowed references", meaning we do not take ownership
and do not need to call Py_INCREF/Py_DECREF on the sequence elements.

2. Memory Handling Strategy:
To avoid unnecessary memory allocations, we DO NOT copy the Python
list into a C array (no malloc/calloc). We iterate over the Python
sequence in-place. Auxiliary space complexity is O(1).

3. Numerical Stability:
For standard deviation, we use Welford's one-pass algorithm.
The naive formula (sum_sq/N - mean^2) is susceptible to catastrophic 
cancellation if values are large and variance is small. Welford's 
method avoids this and computes stable variance in a single O(N) pass.

4. Time Complexity:
All functions operate in strictly O(N) time complexity, where N is 
the length of the input sequence. */




#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>


// Validates input type and gets sequence size
static int get_sequence_info(PyObject *seq, Py_ssize_t *size, int *is_list) {
    if (PyList_Check(seq)) {
        *size = PyList_Size(seq);
        *is_list = 1;
        return 1;
    }
    if (PyTuple_Check(seq)) {
        *size = PyTuple_Size(seq);
        *is_list = 0;
        return 1;
    }
    PyErr_SetString(PyExc_TypeError, "Input data must be a list or tuple.");
    return 0;
}

// Extracts a double from the sequence at a specific index 
static int get_double_item(PyObject *seq, Py_ssize_t index, int is_list, double *out_val) {
    PyObject *item;
    if (is_list) {
        item = PyList_GetItem(seq, index);
    } else {
        item = PyTuple_GetItem(seq, index); 
    }

    if (PyFloat_Check(item)) {
        *out_val = PyFloat_AS_DOUBLE(item);
        return 1;
    } else if (PyLong_Check(item)) {
        *out_val = PyLong_AsDouble(item);
        return 1;
    }

    PyErr_SetString(PyExc_TypeError, "All elements must be floats or integers.");
    return 0;
}

// Function: peak_to_peak(data)
// Math: max(data) - min(data)
static PyObject* vibration_peak_to_peak(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    Py_ssize_t size;
    int is_list;
    if (!get_sequence_info(data, &size, &is_list)) return NULL;

    if (size == 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot calculate peak-to-peak on an empty sequence.");
        return NULL;
    }

    double min_val = 0.0, max_val = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) {
        double val;
        if (!get_double_item(data, i, is_list, &val)) return NULL;
        
        if (i == 0) {
            min_val = val;
            max_val = val;
        } else {
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
    }

    return PyFloat_FromDouble(max_val - min_val);
}

// Function: rms(data)
// Math: sqrt( (1/N) * sum(x_i^2) )
static PyObject* vibration_rms(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    Py_ssize_t size;
    int is_list;
    if (!get_sequence_info(data, &size, &is_list)) return NULL;

    if (size == 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot calculate RMS on an empty sequence.");
        return NULL;
    }

    double sum_sq = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) {
        double val;
        if (!get_double_item(data, i, is_list, &val)) return NULL;
        sum_sq += val * val;
    }

    return PyFloat_FromDouble(sqrt(sum_sq / size));
}

// Function: std_dev(data)
// Math:Welford's Algorithm
static PyObject* vibration_std_dev(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    Py_ssize_t size;
    int is_list;
    if (!get_sequence_info(data, &size, &is_list)) return NULL;

    if (size < 2) {
        PyErr_SetString(PyExc_ValueError, "Sample standard deviation requires at least 2 data points.");
        return NULL;
    }

    double mean = 0.0;
    double M2 = 0.0;

    for (Py_ssize_t i = 0; i < size; i++) {
        double val;
        if (!get_double_item(data, i, is_list, &val)) return NULL;
        
        double delta = val - mean;
        mean += delta / (i + 1);
        double delta2 = val - mean;
        M2 += delta * delta2;
    }

    double variance = M2 / (size - 1);
    return PyFloat_FromDouble(sqrt(variance));
}

// Function: above_threshold(data, threshold)
// Math: Count(x_i > threshold)
static PyObject* vibration_above_threshold(PyObject *self, PyObject *args) {
    PyObject *data;
    double threshold;
    if (!PyArg_ParseTuple(args, "Od", &data, &threshold)) return NULL;

    Py_ssize_t size;
    int is_list;
    if (!get_sequence_info(data, &size, &is_list)) return NULL;

    long count = 0;
    for (Py_ssize_t i = 0; i < size; i++) {
        double val;
        if (!get_double_item(data, i, is_list, &val)) return NULL;
        if (val > threshold) {
            count++;
        }
    }

    return PyLong_FromLong(count);
}

// Function: summary(data)
// Returns a dictionary containing count, mean, min, and max.
static PyObject* vibration_summary(PyObject *self, PyObject *args) {
    PyObject *data;
    if (!PyArg_ParseTuple(args, "O", &data)) return NULL;

    Py_ssize_t size;
    int is_list;
    if (!get_sequence_info(data, &size, &is_list)) return NULL;

    if (size == 0) {
        PyErr_SetString(PyExc_ValueError, "Cannot summarize an empty sequence.");
        return NULL;
    }

    double sum = 0.0, min_val = 0.0, max_val = 0.0;
    for (Py_ssize_t i = 0; i < size; i++) {
        double val;
        if (!get_double_item(data, i, is_list, &val)) return NULL;
        
        sum += val;
        if (i == 0) {
            min_val = val;
            max_val = val;
        } else {
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
        }
    }

    double mean = sum / size;

    // Create dictionary
    PyObject *dict = PyDict_New();
    if (!dict) return NULL;

    // Create Python objects for dictionary values
    PyObject *p_count = PyLong_FromSsize_t(size);
    PyObject *p_mean = PyFloat_FromDouble(mean);
    PyObject *p_min = PyFloat_FromDouble(min_val);
    PyObject *p_max = PyFloat_FromDouble(max_val);

    // Insert into dictionary.
    PyDict_SetItemString(dict, "count", p_count);
    PyDict_SetItemString(dict, "mean", p_mean);
    PyDict_SetItemString(dict, "min", p_min);
    PyDict_SetItemString(dict, "max", p_max);

    // Decrement references of our local objects to prevent memory leaks
    Py_DECREF(p_count);
    Py_DECREF(p_mean);
    Py_DECREF(p_min);
    Py_DECREF(p_max);

    return dict;
}

// Method table 
static PyMethodDef VibrationMethods[] = {
    {"peak_to_peak", vibration_peak_to_peak, METH_VARARGS, "Calculate peak-to-peak difference."},
    {"rms", vibration_rms, METH_VARARGS, "Calculate Root Mean Square (RMS)."},
    {"std_dev", vibration_std_dev, METH_VARARGS, "Calculate sample standard deviation."},
    {"above_threshold", vibration_above_threshold, METH_VARARGS, "Count readings strictly above a threshold."},
    {"summary", vibration_summary, METH_VARARGS, "Return dictionary with count, mean, min, and max."},
    {NULL, NULL, 0, NULL}
};

// Module definition
static struct PyModuleDef vibrationmodule = {
    PyModuleDef_HEAD_INIT,
    "vibration",
    "C extension for rapid industrial vibration analysis.",
    -1,
    VibrationMethods
};

// Initialization function
PyMODINIT_FUNC PyInit_vibration(void) {
    return PyModule_Create(&vibrationmodule);
}


