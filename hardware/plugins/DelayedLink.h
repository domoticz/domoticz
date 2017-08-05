#pragma once

#ifdef WIN32
#	define MS_NO_COREDLL 1
#else
#	pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#ifdef WITH_THREAD
#    undefine WITH_THREAD
#endif
#include <Python.h>
#include <structmember.h>
#include <frameobject.h>
#include "../../main/Helper.h"

namespace Plugins {

#ifdef WIN32
#	define PYTHON_CALL __cdecl*
#	define RESOLVE_SYMBOL GetProcAddress
#else
#	include <dlfcn.h>
#	define PYTHON_CALL *
#	define RESOLVE_SYMBOL dlsym
#endif

#define COMMA ,
#define DECLARE_PYTHON_SYMBOL(type, symbol, params)	typedef type (PYTHON_CALL symbol##_t)(params); symbol##_t  symbol
#define RESOLVE_PYTHON_SYMBOL(symbol)  symbol = (symbol##_t)RESOLVE_SYMBOL(shared_lib_, #symbol)

	struct SharedLibraryProxy
	{
#ifdef WIN32
		HINSTANCE shared_lib_;
#else
		void* shared_lib_;
#endif

		// Shared library interface begin.
		DECLARE_PYTHON_SYMBOL(const char*, Py_GetVersion, );
		DECLARE_PYTHON_SYMBOL(int, Py_IsInitialized, );
		DECLARE_PYTHON_SYMBOL(void, Py_Initialize, );
		DECLARE_PYTHON_SYMBOL(void, Py_Finalize, );
		DECLARE_PYTHON_SYMBOL(PyThreadState*, Py_NewInterpreter, );
		DECLARE_PYTHON_SYMBOL(void, Py_EndInterpreter, PyThreadState*);
		DECLARE_PYTHON_SYMBOL(wchar_t*, Py_GetPath, );
		DECLARE_PYTHON_SYMBOL(void, Py_SetPath, const wchar_t*);
		DECLARE_PYTHON_SYMBOL(void, PySys_SetPath, const wchar_t*);
		DECLARE_PYTHON_SYMBOL(void, Py_SetProgramName, wchar_t*);
		DECLARE_PYTHON_SYMBOL(wchar_t*, Py_GetProgramFullPath, );
		DECLARE_PYTHON_SYMBOL(int, PyImport_AppendInittab, const char* COMMA PyObject* (*initfunc)(void));
		DECLARE_PYTHON_SYMBOL(int, PyType_Ready, PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(int, PyCallable_Check, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_GetAttrString, PyObject* pObj COMMA const char*);
		DECLARE_PYTHON_SYMBOL(int, PyObject_HasAttrString, PyObject* COMMA const char *);
		DECLARE_PYTHON_SYMBOL(const char*, PyBytes_AsString, PyObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyBytes_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_AsASCIIString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromString, const char*);
		DECLARE_PYTHON_SYMBOL(wchar_t*, PyUnicode_AsWideCharString, PyObject* COMMA Py_ssize_t*);
		DECLARE_PYTHON_SYMBOL(const char*, PyUnicode_AsUTF8, PyObject*);
		DECLARE_PYTHON_SYMBOL(char*, PyByteArray_AsString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromKindAndData, int COMMA const void* COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyLong_FromLong, long);
		DECLARE_PYTHON_SYMBOL(PY_LONG_LONG, PyLong_AsLongLong, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_GetDict, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyDict_New, );
		DECLARE_PYTHON_SYMBOL(void, PyDict_Clear, PyObject *);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyDict_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyDict_GetItemString, PyObject* COMMA const char*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItemString, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItem, PyObject* COMMA PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_DelItem, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_Next, PyObject* COMMA Py_ssize_t* COMMA PyObject** COMMA PyObject**);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyDict_Items, PyObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyList_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyList_GetItem, PyObject* COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(void*, PyModule_GetState, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyState_FindModule, struct PyModuleDef*);
		DECLARE_PYTHON_SYMBOL(void, PyErr_Clear, );
		DECLARE_PYTHON_SYMBOL(void, PyErr_Fetch, PyObject** COMMA PyObject** COMMA PyObject**);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyImport_ImportModule, const char*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_CallObject, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyFrame_GetLineNumber, PyFrameObject*);
		DECLARE_PYTHON_SYMBOL(PyThreadState*, PyEval_SaveThread, void);
		DECLARE_PYTHON_SYMBOL(void, PyEval_RestoreThread, PyThreadState*);
		DECLARE_PYTHON_SYMBOL(void, _Py_NegativeRefcount, const char* COMMA int COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, _PyObject_New, PyTypeObject*);
#ifdef _DEBUG
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_Create2TraceRefs, struct PyModuleDef* COMMA int);
#else
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_Create2, struct PyModuleDef* COMMA int);
#endif
		DECLARE_PYTHON_SYMBOL(int, PyModule_AddObject, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTuple, PyObject* COMMA const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTupleAndKeywords, PyObject* COMMA PyObject* COMMA const char* COMMA char*[] COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromFormat, const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, Py_BuildValue, const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(void, PyMem_Free, void*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyBool_FromLong, long);
        DECLARE_PYTHON_SYMBOL(int, PyRun_SimpleStringFlags, const char* COMMA PyCompilerFlags*);
        DECLARE_PYTHON_SYMBOL(int, PyRun_SimpleFileExFlags, FILE* COMMA const char* COMMA int COMMA PyCompilerFlags*);
		DECLARE_PYTHON_SYMBOL(void*, PyCapsule_Import, const char *name COMMA int);
		DECLARE_PYTHON_SYMBOL(void*, PyType_GenericAlloc, const PyTypeObject * COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_DecodeUTF8, const char * COMMA Py_ssize_t COMMA const char *);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyUnicode_GetLength, PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyType_IsSubtype, PyTypeObject* COMMA PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyByteArray_Size, PyObject*);

#ifdef _DEBUG
		// In a debug build dealloc is a function but for release builds its a macro
		DECLARE_PYTHON_SYMBOL(void, _Py_Dealloc, PyObject*);
#endif
		Py_ssize_t		_Py_RefTotal;
		PyObject		_Py_NoneStruct;

		SharedLibraryProxy() {
			shared_lib_ = 0;
			_Py_RefTotal = 0;
			if (!shared_lib_) {
#ifdef WIN32
#	ifdef _DEBUG
				if (!shared_lib_) shared_lib_ = LoadLibrary("python37_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python36_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python35_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python34_d.dll");
#	else
				if (!shared_lib_) shared_lib_ = LoadLibrary("python37.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python36.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python35.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python34.dll");
#	endif
#else
				if (!shared_lib_) FindLibrary("python3.7", true);
				if (!shared_lib_) FindLibrary("python3.6", true);
				if (!shared_lib_) FindLibrary("python3.5", true);
				if (!shared_lib_) FindLibrary("python3.4", true);
#endif
				if (shared_lib_)
				{
					RESOLVE_PYTHON_SYMBOL(Py_GetVersion);
					RESOLVE_PYTHON_SYMBOL(Py_IsInitialized);
					RESOLVE_PYTHON_SYMBOL(Py_Initialize);
					RESOLVE_PYTHON_SYMBOL(Py_Finalize);
					RESOLVE_PYTHON_SYMBOL(Py_NewInterpreter);
					RESOLVE_PYTHON_SYMBOL(Py_EndInterpreter);
					RESOLVE_PYTHON_SYMBOL(Py_GetPath);
					RESOLVE_PYTHON_SYMBOL(Py_SetPath);
					RESOLVE_PYTHON_SYMBOL(PySys_SetPath);
					RESOLVE_PYTHON_SYMBOL(Py_SetProgramName);
					RESOLVE_PYTHON_SYMBOL(Py_GetProgramFullPath);
					RESOLVE_PYTHON_SYMBOL(PyImport_AppendInittab);
					RESOLVE_PYTHON_SYMBOL(PyType_Ready);
					RESOLVE_PYTHON_SYMBOL(PyCallable_Check);
					RESOLVE_PYTHON_SYMBOL(PyObject_GetAttrString);
					RESOLVE_PYTHON_SYMBOL(PyObject_HasAttrString);
					RESOLVE_PYTHON_SYMBOL(PyBytes_AsString);
					RESOLVE_PYTHON_SYMBOL(PyBytes_Size);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsASCIIString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsWideCharString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsUTF8);
					RESOLVE_PYTHON_SYMBOL(PyByteArray_AsString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromKindAndData);
					RESOLVE_PYTHON_SYMBOL(PyLong_FromLong);
					RESOLVE_PYTHON_SYMBOL(PyLong_AsLongLong);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetDict);
					RESOLVE_PYTHON_SYMBOL(PyDict_New);
					RESOLVE_PYTHON_SYMBOL(PyDict_Clear);
					RESOLVE_PYTHON_SYMBOL(PyDict_Size);
					RESOLVE_PYTHON_SYMBOL(PyDict_GetItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItem);
					RESOLVE_PYTHON_SYMBOL(PyDict_DelItem);
					RESOLVE_PYTHON_SYMBOL(PyDict_Next);
					RESOLVE_PYTHON_SYMBOL(PyDict_Items);
					RESOLVE_PYTHON_SYMBOL(PyList_Size);
					RESOLVE_PYTHON_SYMBOL(PyList_GetItem);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetState);
					RESOLVE_PYTHON_SYMBOL(PyState_FindModule);
					RESOLVE_PYTHON_SYMBOL(PyErr_Clear);
					RESOLVE_PYTHON_SYMBOL(PyErr_Fetch);
					RESOLVE_PYTHON_SYMBOL(PyImport_ImportModule);
					RESOLVE_PYTHON_SYMBOL(PyObject_CallObject);
					RESOLVE_PYTHON_SYMBOL(PyFrame_GetLineNumber);
					RESOLVE_PYTHON_SYMBOL(PyEval_SaveThread);
					RESOLVE_PYTHON_SYMBOL(PyEval_RestoreThread);
					RESOLVE_PYTHON_SYMBOL(_Py_NegativeRefcount);
					RESOLVE_PYTHON_SYMBOL(_PyObject_New);
#ifdef _DEBUG
					RESOLVE_PYTHON_SYMBOL(PyModule_Create2TraceRefs);
#else
					RESOLVE_PYTHON_SYMBOL(PyModule_Create2);
#endif
					RESOLVE_PYTHON_SYMBOL(PyModule_AddObject);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTuple);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTupleAndKeywords);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromFormat);
					RESOLVE_PYTHON_SYMBOL(Py_BuildValue);
					RESOLVE_PYTHON_SYMBOL(PyMem_Free);
#ifdef _DEBUG
					RESOLVE_PYTHON_SYMBOL(_Py_Dealloc);
#endif
                    RESOLVE_PYTHON_SYMBOL(PyRun_SimpleFileExFlags);
                    RESOLVE_PYTHON_SYMBOL(PyRun_SimpleStringFlags);
					RESOLVE_PYTHON_SYMBOL(PyBool_FromLong);
					RESOLVE_PYTHON_SYMBOL(PyCapsule_Import);
					RESOLVE_PYTHON_SYMBOL(PyType_GenericAlloc);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_DecodeUTF8);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_GetLength);
					RESOLVE_PYTHON_SYMBOL(PyType_IsSubtype);
					RESOLVE_PYTHON_SYMBOL(PyByteArray_Size);
				}
			}
			_Py_NoneStruct.ob_refcnt = 1;
		};
		~SharedLibraryProxy() {};

		bool Py_LoadLibrary() { return (shared_lib_ != 0); };

#ifndef WIN32
		private:
			void FindLibrary(const std::string sLibrary, bool bSimple = false)
			{
				std::string library;
				if (bSimple)
				{
					// look in directories covered by ldconfig
					if (!shared_lib_)
					{
						library = "lib" + sLibrary + ".so";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
					// look in directories covered by ldconfig but 'm' variant
					if (!shared_lib_)
					{
						library = "lib" + sLibrary + "m.so";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
					// look in /usr/lib directories
					if (!shared_lib_)
					{
						library = "/usr/lib/" + sLibrary + "/";
						FindLibrary(library, false);
					}
					// look in /usr/lib directories but 'm' variant
					if (!shared_lib_)
					{
						library = "/usr/lib/" + sLibrary + "m/";
						FindLibrary(library, false);
					}
					// look in /usr/local/lib directory (handles build from source)
					if (!shared_lib_)
					{
						library = "/usr/local/lib/lib" + sLibrary + ".so";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);

					}
					// look in /usr/local/lib directory (handles build from source) but 'm' variant
					if (!shared_lib_)
					{
						library = "/usr/local/lib/lib" + sLibrary + "m.so";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
				}
				else
				{
					std::vector<std::string> entries;
					std::vector<std::string>::const_iterator itt;
					DirectoryListing(entries, sLibrary, true, false);
					for (itt = entries.begin(); !shared_lib_ && itt != entries.end(); ++itt)
					{
						library = sLibrary + *itt + "/";
						FindLibrary(library, false);
					}

					std::string filename;
					entries.clear();
					DirectoryListing(entries, sLibrary, false, true);
					for (itt = entries.begin(); !shared_lib_ && itt != entries.end(); ++itt)
					{
						filename = *itt;
						if (filename.length() > 12 &&
							filename.compare(0, 11, "libpython3.") == 0 &&
							filename.compare(filename.length() - 3, 3, ".so") == 0 &&
							filename.compare(filename.length() - 6, 6, ".dylib") == 0)
						{
							library = sLibrary + filename;
							shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
						}

					}
				}
			}
#endif
	};

extern	SharedLibraryProxy* pythonLib;

#define	Py_LoadLibrary			pythonLib->Py_LoadLibrary
#define	Py_GetVersion			pythonLib->Py_GetVersion
#define	Py_IsInitialized		pythonLib->Py_IsInitialized
#define	Py_Initialize			pythonLib->Py_Initialize
#define	Py_Finalize				pythonLib->Py_Finalize
#define	Py_NewInterpreter		pythonLib->Py_NewInterpreter
#define	Py_EndInterpreter		pythonLib->Py_EndInterpreter
#define	Py_SetPath				pythonLib->Py_SetPath
#define	PySys_SetPath			pythonLib->PySys_SetPath
#define	Py_GetPath				pythonLib->Py_GetPath
#define	Py_SetProgramName		pythonLib->Py_SetProgramName
#define	Py_GetProgramFullPath	pythonLib->Py_GetProgramFullPath
#define	PyImport_AppendInittab	pythonLib->PyImport_AppendInittab
#define	PyType_Ready			pythonLib->PyType_Ready
#define	PyCallable_Check		pythonLib->PyCallable_Check
#define	PyObject_GetAttrString	pythonLib->PyObject_GetAttrString
#define	PyObject_HasAttrString	pythonLib->PyObject_HasAttrString
#define	PyBytes_AsString		pythonLib->PyBytes_AsString
#define	PyBytes_Size			pythonLib->PyBytes_Size
#define PyUnicode_AsASCIIString pythonLib->PyUnicode_AsASCIIString
#define PyUnicode_FromString	pythonLib->PyUnicode_FromString
#define PyUnicode_FromFormat	pythonLib->PyUnicode_FromFormat
#define PyUnicode_AsWideCharString	pythonLib->PyUnicode_AsWideCharString
#define PyUnicode_AsUTF8		pythonLib->PyUnicode_AsUTF8
#define PyByteArray_AsString	pythonLib->PyByteArray_AsString
#define PyUnicode_FromKindAndData  pythonLib->PyUnicode_FromKindAndData
#define PyLong_FromLong			pythonLib->PyLong_FromLong
#define PyLong_AsLongLong		pythonLib->PyLong_AsLongLong
#define PyModule_GetDict		pythonLib->PyModule_GetDict
#define PyDict_New				pythonLib->PyDict_New
#define PyDict_Clear			pythonLib->PyDict_Clear
#define PyDict_Size				pythonLib->PyDict_Size
#define PyDict_GetItemString	pythonLib->PyDict_GetItemString
#define PyDict_SetItemString	pythonLib->PyDict_SetItemString
#define PyDict_SetItem			pythonLib->PyDict_SetItem
#define PyDict_DelItem			pythonLib->PyDict_DelItem
#define PyDict_Next				pythonLib->PyDict_Next
#define PyDict_Items			pythonLib->PyDict_Items
#define PyList_Size				pythonLib->PyList_Size
#define PyList_GetItem			pythonLib->PyList_GetItem
#define PyModule_GetState		pythonLib->PyModule_GetState
#define PyState_FindModule		pythonLib->PyState_FindModule
#define PyErr_Clear				pythonLib->PyErr_Clear
#define PyErr_Fetch				pythonLib->PyErr_Fetch
#define PyImport_ImportModule	pythonLib->PyImport_ImportModule
#define PyObject_CallObject		pythonLib->PyObject_CallObject
#define PyFrame_GetLineNumber	pythonLib->PyFrame_GetLineNumber
#define PyEval_SaveThread		pythonLib->PyEval_SaveThread
#define PyEval_RestoreThread	pythonLib->PyEval_RestoreThread
#define _Py_NegativeRefcount	pythonLib->_Py_NegativeRefcount
#define _PyObject_New			pythonLib->_PyObject_New
#define PyArg_ParseTuple		pythonLib->PyArg_ParseTuple
#define Py_BuildValue			pythonLib->Py_BuildValue
#define PyMem_Free				pythonLib->PyMem_Free
#ifdef _DEBUG
#	define PyModule_Create2TraceRefs pythonLib->PyModule_Create2TraceRefs
#else
#	define PyModule_Create2		pythonLib->PyModule_Create2
#endif
#define PyModule_AddObject		pythonLib->PyModule_AddObject
#define PyArg_ParseTupleAndKeywords pythonLib->PyArg_ParseTupleAndKeywords

#ifdef _DEBUG
#	define _Py_Dealloc			pythonLib->_Py_Dealloc
#endif

#define _Py_RefTotal			pythonLib->_Py_RefTotal
#define _Py_NoneStruct			pythonLib->_Py_NoneStruct
#define PyRun_SimpleStringFlags pythonLib->PyRun_SimpleStringFlags
#define PyRun_SimpleFileExFlags pythonLib->PyRun_SimpleFileExFlags
#define PyBool_FromLong			pythonLib->PyBool_FromLong
#define PyCapsule_Import		pythonLib->PyCapsule_Import
#define PyType_GenericAlloc		pythonLib->PyType_GenericAlloc
#define PyUnicode_DecodeUTF8	pythonLib->PyUnicode_DecodeUTF8
#define PyUnicode_GetLength		pythonLib->PyUnicode_GetLength
#define PyType_IsSubtype		pythonLib->PyType_IsSubtype
#define PyByteArray_Size		pythonLib->PyByteArray_Size
}
