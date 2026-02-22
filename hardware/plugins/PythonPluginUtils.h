#pragma once
#ifdef ENABLE_PYTHON

#include "DelayedLink.h"
#include <string>
#include <algorithm>

namespace Plugins {

	// Case-insensitive keyword normalization for PyArg_ParseTupleAndKeywords
	// Returns a new reference to a normalized dictionary, or a borrowed reference if kwds is NULL
	// Caller must check if returned value != kwds and call Py_DECREF on the normalized dict when done
	static PyObject* NormalizeKeywords(PyObject *kwds)
	{
		if (!kwds)
			return kwds;

		PyObject *normalized = PyDict_New();
		if (!normalized)
			return kwds;

		PyObject *key, *value;
		Py_ssize_t pos = 0;

		while (PyDict_Next(kwds, &pos, &key, &value))
		{
			const char *key_str = PyUnicode_AsUTF8(key);
			if (key_str)
			{
				std::string lower_str(key_str);
				std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
				PyObject *lower_key = PyUnicode_FromString(lower_str.c_str());
				if (lower_key)
				{
					PyDict_SetItem(normalized, lower_key, value);
					Py_DECREF(lower_key);
				}
				else
				{
					PyDict_SetItem(normalized, key, value);
				}
			}
			else
			{
				PyErr_Clear();
				PyDict_SetItem(normalized, key, value);
			}
		}

		return normalized;
	}

	// maptypename declaration (defined in PythonObjects.cpp)
	extern void maptypename(const std::string &sTypeName, int &Type, int &SubType, int &SwitchType, std::string &sValue, PyObject *OptionsIn, PyObject *OptionsOut);

} // namespace Plugins

#endif
