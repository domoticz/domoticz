#pragma once
#ifdef ENABLE_PYTHON
#ifdef WIN32
#	define MS_NO_COREDLL 1
#else
#	pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

#ifdef WITH_THREAD
#    undefine WITH_THREAD
#endif

#pragma push_macro("_DEBUG")
#ifdef _DEBUG
#    undef _DEBUG   // Not compatible with Py_LIMITED_API
#endif
#define Py_LIMITED_API 0x03040000 
#include <Python.h>
#include <structmember.h>
#include "../../main/Logger.h"

#ifndef WIN32
#	include "../../main/Helper.h"
#endif

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

#undef Py_None

	struct SharedLibraryProxy
	{
#ifdef WIN32
		HINSTANCE shared_lib_;
#else
		void* shared_lib_;
#endif
		PyObject* Py_None;
			
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
		DECLARE_PYTHON_SYMBOL(int, PyImport_AppendInittab, const char *COMMA PyObject *(*initfunc)());
		DECLARE_PYTHON_SYMBOL(int, PyType_Ready, PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_Type, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyType_FromSpec, PyType_Spec*);
		DECLARE_PYTHON_SYMBOL(void*, PyType_GetSlot, PyTypeObject* COMMA int);
		DECLARE_PYTHON_SYMBOL(int, PyCallable_Check, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_GetAttrString, PyObject* pObj COMMA const char*);
		DECLARE_PYTHON_SYMBOL(int, PyObject_HasAttrString, PyObject* COMMA const char *);
		DECLARE_PYTHON_SYMBOL(const char*, PyBytes_AsString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyBytes_FromStringAndSize, const char* COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyBytes_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_AsASCIIString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromString, const char*);
		DECLARE_PYTHON_SYMBOL(wchar_t*, PyUnicode_AsWideCharString, PyObject* COMMA Py_ssize_t*);
		DECLARE_PYTHON_SYMBOL(const char*, PyUnicode_AsUTF8, PyObject*);
		DECLARE_PYTHON_SYMBOL(char*, PyByteArray_AsString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyLong_FromLong, long);
		DECLARE_PYTHON_SYMBOL(PY_LONG_LONG, PyLong_AsLongLong, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_GetDict, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyDict_New, );
		DECLARE_PYTHON_SYMBOL(void, PyDict_Clear, PyObject *);
		DECLARE_PYTHON_SYMBOL(int, PyDict_Contains, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyDict_Size, PyObject *);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyDict_GetItem, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyDict_GetItemString, PyObject* COMMA const char*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItemString, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItem, PyObject* COMMA PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_DelItem, PyObject *COMMA PyObject *);
		DECLARE_PYTHON_SYMBOL(int, PyDict_DelItemString, PyObject *COMMA const char *);
		DECLARE_PYTHON_SYMBOL(int, PyDict_Next, PyObject *COMMA Py_ssize_t *COMMA PyObject **COMMA PyObject **);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyDict_Items, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyList_New, Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyList_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyTuple_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyList_Append, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyList_GetItem, PyObject* COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyTuple_GetItem, PyObject* COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(int, PyList_SetItem, PyObject* COMMA Py_ssize_t COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(void*, PyModule_GetState, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyState_FindModule, struct PyModuleDef*);
		DECLARE_PYTHON_SYMBOL(void, PyErr_Clear, );
		DECLARE_PYTHON_SYMBOL(void, PyErr_Fetch, PyObject** COMMA PyObject** COMMA PyObject**);
		DECLARE_PYTHON_SYMBOL(void, PyErr_NormalizeException, PyObject **COMMA PyObject **COMMA PyObject **);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyImport_ImportModule, const char *);
		DECLARE_PYTHON_SYMBOL(int, PyObject_RichCompareBool, PyObject* COMMA PyObject* COMMA int);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyObject_CallObject, PyObject *COMMA PyObject *);
		DECLARE_PYTHON_SYMBOL(void, PyEval_InitThreads, );
		DECLARE_PYTHON_SYMBOL(int, PyEval_ThreadsInitialized, );
		DECLARE_PYTHON_SYMBOL(PyThreadState*, PyThreadState_Get, );
		DECLARE_PYTHON_SYMBOL(PyThreadState *, PyEval_SaveThread, void);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyEval_GetLocals, void);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyEval_GetGlobals, void);
		DECLARE_PYTHON_SYMBOL(void, PyEval_RestoreThread, PyThreadState *);
		DECLARE_PYTHON_SYMBOL(void, PyEval_ReleaseLock, );
		DECLARE_PYTHON_SYMBOL(PyThreadState*, PyThreadState_Swap, PyThreadState*);
		DECLARE_PYTHON_SYMBOL(void, _Py_NegativeRefcount, const char* COMMA int COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject *, _PyObject_New, PyTypeObject *);
		DECLARE_PYTHON_SYMBOL(int, PyObject_IsInstance, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyObject_IsSubclass, PyObject *COMMA PyObject *);
		DECLARE_PYTHON_SYMBOL(PyObject *, PyObject_Dir, PyObject *);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_Create2, struct PyModuleDef* COMMA int);
		DECLARE_PYTHON_SYMBOL(int, PyModule_AddObject, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTuple, PyObject* COMMA const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTupleAndKeywords, PyObject* COMMA PyObject* COMMA const char* COMMA char*[] COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromFormat, const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, Py_BuildValue, const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(void, PyMem_Free, void*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyBool_FromLong, long);
		DECLARE_PYTHON_SYMBOL(void*, PyCapsule_Import, const char *name COMMA int);
		DECLARE_PYTHON_SYMBOL(void*, PyType_GenericAlloc, const PyTypeObject * COMMA Py_ssize_t);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_DecodeUTF8, const char * COMMA Py_ssize_t COMMA const char *);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyUnicode_GetLength, PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyType_IsSubtype, PyTypeObject* COMMA PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(Py_ssize_t, PyByteArray_Size, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyErr_Occurred, );
		DECLARE_PYTHON_SYMBOL(long, PyLong_AsLong, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_AsUTF8String, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyImport_AddModule, const char*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_Str, PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyObject_IsTrue, PyObject*);
		DECLARE_PYTHON_SYMBOL(double, PyFloat_AsDouble, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_GetIter, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyIter_Next, PyObject*);
		DECLARE_PYTHON_SYMBOL(void, PyErr_SetString, PyObject* COMMA const char*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_CallFunctionObjArgs, PyObject* COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, Py_CompileString, const char* COMMA const char* COMMA int);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyEval_EvalCode, PyObject* COMMA PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(long, PyType_GetFlags, PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(void, _Py_Dealloc, PyObject*);

		SharedLibraryProxy() {
			Py_None = nullptr;
			shared_lib_ = nullptr;
			if (!shared_lib_) {
#ifdef WIN32
				if (!shared_lib_) shared_lib_ = LoadLibrary("python311.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python310.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python39.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python38.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python37.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python36.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python35.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python34.dll");
#else
				if (!shared_lib_) FindLibrary("python3.11", true);
				if (!shared_lib_) FindLibrary("python3.10", true);
				if (!shared_lib_) FindLibrary("python3.9", true);
				if (!shared_lib_) FindLibrary("python3.8", true);
				if (!shared_lib_) FindLibrary("python3.7", true);
				if (!shared_lib_) FindLibrary("python3.6", true);
				if (!shared_lib_) FindLibrary("python3.5", true);
				if (!shared_lib_) FindLibrary("python3.4", true);
#ifdef __FreeBSD__
				if (!shared_lib_) FindLibrary("python3.7m", true);
				if (!shared_lib_) FindLibrary("python3.6m", true);
				if (!shared_lib_) FindLibrary("python3.5m", true);
				if (!shared_lib_) FindLibrary("python3.4m", true);
#endif /* FreeBSD */
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
					RESOLVE_PYTHON_SYMBOL(PyObject_Type);
					RESOLVE_PYTHON_SYMBOL(PyType_FromSpec);
					RESOLVE_PYTHON_SYMBOL(PyType_GetSlot);
					RESOLVE_PYTHON_SYMBOL(PyCallable_Check);
					RESOLVE_PYTHON_SYMBOL(PyObject_GetAttrString);
					RESOLVE_PYTHON_SYMBOL(PyObject_HasAttrString);
					RESOLVE_PYTHON_SYMBOL(PyBytes_AsString);
					RESOLVE_PYTHON_SYMBOL(PyBytes_FromStringAndSize);
					RESOLVE_PYTHON_SYMBOL(PyBytes_Size);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsASCIIString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsWideCharString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsUTF8);
					RESOLVE_PYTHON_SYMBOL(PyByteArray_AsString);
					RESOLVE_PYTHON_SYMBOL(PyLong_FromLong);
					RESOLVE_PYTHON_SYMBOL(PyLong_AsLongLong);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetDict);
					RESOLVE_PYTHON_SYMBOL(PyDict_New);
					RESOLVE_PYTHON_SYMBOL(PyDict_Contains);
					RESOLVE_PYTHON_SYMBOL(PyDict_Clear);
					RESOLVE_PYTHON_SYMBOL(PyDict_Size);
					RESOLVE_PYTHON_SYMBOL(PyDict_GetItem);
					RESOLVE_PYTHON_SYMBOL(PyDict_GetItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItem);
					RESOLVE_PYTHON_SYMBOL(PyDict_DelItem);
					RESOLVE_PYTHON_SYMBOL(PyDict_DelItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_Next);
					RESOLVE_PYTHON_SYMBOL(PyDict_Items);
					RESOLVE_PYTHON_SYMBOL(PyList_New);
					RESOLVE_PYTHON_SYMBOL(PyList_Size);
					RESOLVE_PYTHON_SYMBOL(PyTuple_Size);
					RESOLVE_PYTHON_SYMBOL(PyList_GetItem);
					RESOLVE_PYTHON_SYMBOL(PyTuple_GetItem);
					RESOLVE_PYTHON_SYMBOL(PyList_SetItem);
					RESOLVE_PYTHON_SYMBOL(PyList_Append);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetState);
					RESOLVE_PYTHON_SYMBOL(PyState_FindModule);
					RESOLVE_PYTHON_SYMBOL(PyErr_Clear);
					RESOLVE_PYTHON_SYMBOL(PyErr_Fetch);
					RESOLVE_PYTHON_SYMBOL(PyErr_NormalizeException);
					RESOLVE_PYTHON_SYMBOL(PyImport_ImportModule);
					RESOLVE_PYTHON_SYMBOL(PyObject_RichCompareBool);
					RESOLVE_PYTHON_SYMBOL(PyObject_CallObject);
					RESOLVE_PYTHON_SYMBOL(PyEval_InitThreads);
					RESOLVE_PYTHON_SYMBOL(PyEval_ThreadsInitialized);
					RESOLVE_PYTHON_SYMBOL(PyThreadState_Get);
					RESOLVE_PYTHON_SYMBOL(PyEval_SaveThread);
					RESOLVE_PYTHON_SYMBOL(PyEval_GetLocals);
					RESOLVE_PYTHON_SYMBOL(PyEval_GetGlobals);
					RESOLVE_PYTHON_SYMBOL(PyEval_RestoreThread);
					RESOLVE_PYTHON_SYMBOL(PyEval_ReleaseLock);
					RESOLVE_PYTHON_SYMBOL(PyThreadState_Swap);
					RESOLVE_PYTHON_SYMBOL(_Py_NegativeRefcount);
					RESOLVE_PYTHON_SYMBOL(_PyObject_New);
					RESOLVE_PYTHON_SYMBOL(PyObject_IsInstance);
					RESOLVE_PYTHON_SYMBOL(PyObject_IsSubclass);
					RESOLVE_PYTHON_SYMBOL(PyObject_Dir);
					RESOLVE_PYTHON_SYMBOL(PyModule_Create2);
					RESOLVE_PYTHON_SYMBOL(PyModule_AddObject);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTuple);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTupleAndKeywords);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromFormat);
					RESOLVE_PYTHON_SYMBOL(Py_BuildValue);
					RESOLVE_PYTHON_SYMBOL(PyMem_Free);
					RESOLVE_PYTHON_SYMBOL(PyBool_FromLong);
					RESOLVE_PYTHON_SYMBOL(PyCapsule_Import);
					RESOLVE_PYTHON_SYMBOL(PyType_GenericAlloc);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_DecodeUTF8);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_GetLength);
					RESOLVE_PYTHON_SYMBOL(PyType_IsSubtype);
					RESOLVE_PYTHON_SYMBOL(PyByteArray_Size);
					RESOLVE_PYTHON_SYMBOL(PyErr_Occurred);
					RESOLVE_PYTHON_SYMBOL(PyLong_AsLong);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsUTF8String);
					RESOLVE_PYTHON_SYMBOL(PyImport_AddModule);
					RESOLVE_PYTHON_SYMBOL(PyObject_Str);
					RESOLVE_PYTHON_SYMBOL(PyObject_IsTrue);
					RESOLVE_PYTHON_SYMBOL(PyFloat_AsDouble);
					RESOLVE_PYTHON_SYMBOL(PyObject_GetIter);
					RESOLVE_PYTHON_SYMBOL(PyIter_Next);
					RESOLVE_PYTHON_SYMBOL(PyErr_SetString);
					RESOLVE_PYTHON_SYMBOL(PyObject_CallFunctionObjArgs);
					RESOLVE_PYTHON_SYMBOL(Py_CompileString);
					RESOLVE_PYTHON_SYMBOL(PyEval_EvalCode);
					RESOLVE_PYTHON_SYMBOL(PyType_GetFlags);
					RESOLVE_PYTHON_SYMBOL(_Py_Dealloc);
				}
			}
		};
		~SharedLibraryProxy() = default;

		bool Py_LoadLibrary()
		{
			return (shared_lib_ != nullptr);
		};

#ifndef WIN32
		private:
			void FindLibrary(const std::string &sLibrary, bool bSimple = false)
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
					// MacOS
					// look for .dylib in /usr/local/lib
					if (!shared_lib_)
					{
						library = "/usr/local/lib/lib" + sLibrary + ".dylib";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
					// look for .dylib in /Library/Frameworks/Python.framework/Versions/*/lib
					if (!shared_lib_)
					{
						library = "/Library/Frameworks/Python.framework/Versions/"+sLibrary.substr(sLibrary.size() - 3)+"/lib/lib" + sLibrary + ".dylib";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
					// look for .dylib installed by Homebrew - Intel
					if (!shared_lib_)
					{
						library = "/usr/local/Frameworks/Python.framework/Versions/"+sLibrary.substr(sLibrary.size() - 3)+"/lib/lib" + sLibrary + ".dylib";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
					// look for .dylib installed by Homebrew - Apple Silicon
					if (!shared_lib_)
					{
						library = "/opt/homebrew/Frameworks/Python.framework/Versions/"+sLibrary.substr(sLibrary.size() - 3)+"/lib/lib" + sLibrary + ".dylib";
						shared_lib_ = dlopen(library.c_str(), RTLD_LAZY | RTLD_GLOBAL);
					}
				}
				else
				{
					std::vector<std::string> entries;
					DirectoryListing(entries, sLibrary, true, false);
					for (const auto &entry : entries)
					{
						if (shared_lib_)
						{
							break;
						}

						library = sLibrary + entry + "/";
						FindLibrary(library, false);
					}

					entries.clear();
					DirectoryListing(entries, sLibrary, false, true);
					for (const auto &filename : entries)
					{
						if (shared_lib_)
						{
							break;
						}

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

#define	Py_None					pythonLib->Py_None
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
#define	PyObject_Type			pythonLib->PyObject_Type
#define	PyType_FromSpec			pythonLib->PyType_FromSpec
#define	PyType_GetSlot			pythonLib->PyType_GetSlot
#define	PyCallable_Check		pythonLib->PyCallable_Check
#define	PyObject_GetAttrString	pythonLib->PyObject_GetAttrString
#define	PyObject_HasAttrString	pythonLib->PyObject_HasAttrString
#define	PyBytes_AsString		pythonLib->PyBytes_AsString
#define	PyBytes_FromStringAndSize	pythonLib->PyBytes_FromStringAndSize
#define	PyBytes_Size			pythonLib->PyBytes_Size
#define PyUnicode_AsASCIIString pythonLib->PyUnicode_AsASCIIString
#define PyUnicode_FromString	pythonLib->PyUnicode_FromString
#define PyUnicode_FromFormat	pythonLib->PyUnicode_FromFormat
#define PyUnicode_AsWideCharString	pythonLib->PyUnicode_AsWideCharString
#define PyUnicode_AsUTF8		pythonLib->PyUnicode_AsUTF8
#define PyByteArray_AsString	pythonLib->PyByteArray_AsString
#define PyLong_FromLong			pythonLib->PyLong_FromLong
#define PyLong_AsLongLong		pythonLib->PyLong_AsLongLong
#define PyModule_GetDict		pythonLib->PyModule_GetDict
#define PyDict_New				pythonLib->PyDict_New
#define PyDict_Contains			pythonLib->PyDict_Contains
#define PyDict_Clear			pythonLib->PyDict_Clear
#define PyDict_Size				pythonLib->PyDict_Size
#define PyDict_GetItem			pythonLib->PyDict_GetItem
#define PyDict_GetItemString	pythonLib->PyDict_GetItemString
#define PyDict_SetItemString	pythonLib->PyDict_SetItemString
#define PyDict_SetItem			pythonLib->PyDict_SetItem
#define PyDict_DelItem			pythonLib->PyDict_DelItem
#define PyDict_DelItemString	pythonLib->PyDict_DelItemString
#define PyDict_Next				pythonLib->PyDict_Next
#define PyDict_Items			pythonLib->PyDict_Items
#define PyList_New				pythonLib->PyList_New
#define PyList_Size				pythonLib->PyList_Size
#define PyTuple_Size			pythonLib->PyTuple_Size
#define PyList_GetItem			pythonLib->PyList_GetItem
#define PyTuple_GetItem			pythonLib->PyTuple_GetItem
#define PyList_SetItem			pythonLib->PyList_SetItem
#define PyList_Append			pythonLib->PyList_Append
#define PyModule_GetState		pythonLib->PyModule_GetState
#define PyState_FindModule		pythonLib->PyState_FindModule
#define PyErr_Clear				pythonLib->PyErr_Clear
#define PyErr_Fetch				pythonLib->PyErr_Fetch
#define PyErr_NormalizeException pythonLib->PyErr_NormalizeException
#define PyImport_ImportModule	pythonLib->PyImport_ImportModule
#define PyObject_RichCompareBool pythonLib->PyObject_RichCompareBool
#define PyObject_CallObject		pythonLib->PyObject_CallObject
#define	PyEval_InitThreads		pythonLib->PyEval_InitThreads
#define	PyEval_ThreadsInitialized	pythonLib->PyEval_ThreadsInitialized
#define	PyThreadState_Get		pythonLib->PyThreadState_Get
#define PyEval_SaveThread		pythonLib->PyEval_SaveThread
#define PyEval_GetLocals		pythonLib->PyEval_GetLocals
#define PyEval_GetGlobals		pythonLib->PyEval_GetGlobals
#define PyEval_RestoreThread	pythonLib->PyEval_RestoreThread
#define PyEval_ReleaseLock		pythonLib->PyEval_ReleaseLock
#define PyThreadState_Swap		pythonLib->PyThreadState_Swap
#define _Py_NegativeRefcount	pythonLib->_Py_NegativeRefcount
#define _PyObject_New			pythonLib->_PyObject_New
#define PyObject_IsInstance		pythonLib->PyObject_IsInstance
#define PyObject_IsSubclass		pythonLib->PyObject_IsSubclass
#define PyObject_Dir			pythonLib->PyObject_Dir
#define PyArg_ParseTuple		pythonLib->PyArg_ParseTuple
#define Py_BuildValue			pythonLib->Py_BuildValue
#define PyMem_Free				pythonLib->PyMem_Free
#define PyModule_Create2		pythonLib->PyModule_Create2
#define PyModule_AddObject		pythonLib->PyModule_AddObject
#define PyArg_ParseTupleAndKeywords pythonLib->PyArg_ParseTupleAndKeywords
#define _Py_RefTotal			pythonLib->_Py_RefTotal
#define PyBool_FromLong			pythonLib->PyBool_FromLong
#define PyCapsule_Import		pythonLib->PyCapsule_Import
#define PyType_GenericAlloc		pythonLib->PyType_GenericAlloc
#define PyUnicode_DecodeUTF8	pythonLib->PyUnicode_DecodeUTF8
#define PyUnicode_GetLength		pythonLib->PyUnicode_GetLength
#define PyType_IsSubtype		pythonLib->PyType_IsSubtype
#define PyByteArray_Size		pythonLib->PyByteArray_Size
#define PyErr_Occurred			pythonLib->PyErr_Occurred
#define PyLong_AsLong			pythonLib->PyLong_AsLong
#define PyUnicode_AsUTF8String	pythonLib->PyUnicode_AsUTF8String
#define PyImport_AddModule		pythonLib->PyImport_AddModule
#define PyObject_Str			pythonLib->PyObject_Str
#define	PyObject_IsTrue			pythonLib->PyObject_IsTrue
#define PyFloat_AsDouble		pythonLib->PyFloat_AsDouble
#define	PyObject_GetIter		pythonLib->PyObject_GetIter
#define	PyIter_Next				pythonLib->PyIter_Next
#define PyErr_SetString			pythonLib->PyErr_SetString
#define	PyObject_CallFunctionObjArgs	pythonLib->PyObject_CallFunctionObjArgs
#define	Py_CompileString		pythonLib->Py_CompileString
#define	PyEval_EvalCode			pythonLib->PyEval_EvalCode
#define	PyType_GetFlags			pythonLib->PyType_GetFlags
#ifdef WIN32  
#	define	_Py_Dealloc				pythonLib->_Py_Dealloc		// Builds against a low Python version
#elif PY_VERSION_HEX < 0x03090000
#	define	_Py_Dealloc				pythonLib->_Py_Dealloc
#else
#	ifndef _Py_DEC_REFTOTAL
	/* _Py_DEC_REFTOTAL macro has been removed from Python 3.9 by: https://github.com/python/cpython/commit/49932fec62c616ec88da52642339d83ae719e924 */
#		ifdef Py_REF_DEBUG
#			define _Py_DEC_REFTOTAL _Py_RefTotal--
#		else
#			define _Py_DEC_REFTOTAL
#			define _Py_Dealloc
#		endif
#	endif
#endif

#if PY_VERSION_HEX >= 0x030800f0
static inline void py3__Py_INCREF(PyObject* op)
{
#ifdef Py_REF_DEBUG
	_Py_RefTotal++;
#endif
	op->ob_refcnt++;
}

#undef Py_INCREF
#define Py_INCREF(op) py3__Py_INCREF(_PyObject_CAST(op))

static inline void py3__Py_XINCREF(PyObject* op)
{
	if (op != NULL)
	{
		Py_INCREF(op);
	}
}

#undef Py_XINCREF
#define Py_XINCREF(op) py3__Py_XINCREF(_PyObject_CAST(op))

static inline void py3__Py_DECREF(const char* filename, int lineno, PyObject* op)
{
	(void)filename; /* may be unused, shut up -Wunused-parameter */
	(void)lineno;	/* may be unused, shut up -Wunused-parameter */
	_Py_DEC_REFTOTAL;
	if (--op->ob_refcnt != 0)
	{
#ifdef Py_REF_DEBUG
		if (op->ob_refcnt < 0)
		{
			_Py_NegativeRefcount(filename, lineno, op);
		}
#endif
	}
	else
	{
		_Py_Dealloc(op);
	}
}

#undef Py_DECREF
#define Py_DECREF(op) py3__Py_DECREF(__FILE__, __LINE__, _PyObject_CAST(op))

static inline void py3__Py_XDECREF(PyObject* op)
{
	if (op != nullptr)
	{
		Py_DECREF(op);
	}
}

#undef Py_XDECREF
#define Py_XDECREF(op) py3__Py_XDECREF(_PyObject_CAST(op))
#endif
#pragma pop_macro("_DEBUG")
} // namespace Plugins
#endif //#ifdef ENABLE_PYTHON
