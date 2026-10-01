#pragma once
#ifdef ENABLE_PYTHON

#include "DelayedLink.h"
#include "../ColorSwitch.h"
#include "../../main/json_helper.h"
#include <string>
#include <algorithm>

namespace Plugins {

	// DeviceStatus.Color carries two unrelated payloads: RGB/WW color state for color
	// switches, and the bar range definitions rendered by the utility, temperature and
	// weather cards (www/app/widgets/dzBar.js). Only color state may be normalized
	// through _tColor: a range payload carries no color mode, so _tColor would reduce it
	// to the empty string and silently discard it.
	//
	// Color state is normalized as before (preserving the "detect incorrectly formatted
	// color data" behaviour), any other well-formed JSON is stored verbatim, and anything
	// that is not JSON at all is still rejected.
	inline std::string NormalizeDeviceColor(const std::string &sColor)
	{
		if (sColor.empty())
			return "";

		Json::Value root;
		if (!ParseJSonStrict(sColor, root))
			return ""; // Not JSON at all, so incorrectly formatted whichever payload was meant

		// A color mode member is what distinguishes color state from a range definition
		if (root.isObject() && root.isMember("m"))
			return _tColor(root).toJSONString();

		return sColor;
	}

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

	// Wrapper for PyArg_ParseTupleAndKeywords that normalizes keywords to lowercase
	// before parsing, providing case-insensitive keyword argument matching.
	// Usage is identical to PyArg_ParseTupleAndKeywords.
	static int PyArg_ParseTupleAndNormalizedKeywords(PyObject *args, PyObject *kwds, const char *format, char *kwlist[], ...)
	{
		PyObject *normalized_kwds = NormalizeKeywords(kwds);

		va_list va;
		va_start(va, kwlist);
		int result = PyArg_VaParseTupleAndKeywords(args, normalized_kwds, format, kwlist, va);
		va_end(va);

		if (normalized_kwds != kwds)
		{
			Py_DECREF(normalized_kwds);
		}

		return result;
	}

	// maptypename declaration (defined in PythonObjects.cpp)
	extern void maptypename(const std::string &sTypeName, int &Type, int &SubType, int &SwitchType, std::string &sValue, PyObject *OptionsIn, PyObject *OptionsOut);

} // namespace Plugins

#endif
