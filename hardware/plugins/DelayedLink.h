#pragma once

#ifdef WIN32
#	include "../../../domoticz/main/dirent_windows.h"
#else
#	include <dirent.h>
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
		DECLARE_PYTHON_SYMBOL(void, Py_SetProgramName, wchar_t*);
		DECLARE_PYTHON_SYMBOL(wchar_t*, Py_GetProgramFullPath, );
		DECLARE_PYTHON_SYMBOL(int, PyImport_AppendInittab, const char* COMMA PyObject* (*initfunc)(void));
		DECLARE_PYTHON_SYMBOL(int, PyType_Ready, PyTypeObject*);
		DECLARE_PYTHON_SYMBOL(int, PyCallable_Check, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_GetAttrString, PyObject* pObj COMMA const char*);
		DECLARE_PYTHON_SYMBOL(int, PyObject_HasAttrString, PyObject* COMMA const char *);
		DECLARE_PYTHON_SYMBOL(const char*, PyBytes_AsString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_AsASCIIString, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromString, const char*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyLong_FromLong, long);
		DECLARE_PYTHON_SYMBOL(PY_LONG_LONG, PyLong_AsLongLong, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_GetDict, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyDict_New, );
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItemString, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyDict_SetItem, PyObject* COMMA PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(void*, PyModule_GetState, PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyState_FindModule, struct PyModuleDef*);
		DECLARE_PYTHON_SYMBOL(void, PyErr_Clear, );
		DECLARE_PYTHON_SYMBOL(void, PyErr_Fetch, PyObject** COMMA PyObject** COMMA PyObject**);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyImport_ImportModule, const char*);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyObject_CallObject, PyObject* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyFrame_GetLineNumber, PyFrameObject*);
		DECLARE_PYTHON_SYMBOL(void, PyEval_RestoreThread, PyThreadState*);
		DECLARE_PYTHON_SYMBOL(void, _Py_NegativeRefcount, const char* COMMA int COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(PyObject*, _PyObject_New, PyTypeObject*);
#ifdef DEBUG
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_Create2TraceRefs, struct PyModuleDef* COMMA int);
#else
		DECLARE_PYTHON_SYMBOL(PyObject*, PyModule_Create2, struct PyModuleDef* COMMA int);
#endif
		DECLARE_PYTHON_SYMBOL(int, PyModule_AddObject, PyObject* COMMA const char* COMMA PyObject*);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTuple, PyObject* COMMA const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(int, PyArg_ParseTupleAndKeywords, PyObject* COMMA PyObject* COMMA const char* COMMA char*[] COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, PyUnicode_FromFormat, const char* COMMA ...);
		DECLARE_PYTHON_SYMBOL(PyObject*, Py_BuildValue, const char* COMMA ...);

#ifdef DEBUG
		// In a debug build dealloc is a function but for release builds its a macro
		DECLARE_PYTHON_SYMBOL(void, _Py_Dealloc, PyObject*);
#endif
		Py_ssize_t	_Py_RefTotal;
		PyObject	_Py_NoneStruct;

		SharedLibraryProxy() {
			shared_lib_ = 0;
			_Py_RefTotal = 0;
			if (!shared_lib_) {
#ifdef WIN32
#	ifdef DEBUG
				shared_lib_ = LoadLibrary("python36_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python35_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python34_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python33_d.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python32_d.dll");
#	else
				shared_lib_ = LoadLibrary("python36.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python35.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python34.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python33.dll");
				if (!shared_lib_) shared_lib_ = LoadLibrary("python32.dll");
#	endif
#else
				FindLibrary("python3.6", true);
				if (!shared_lib_) FindLibrary("python3.5", true);
				if (!shared_lib_) FindLibrary("python3.4", true);
				if (!shared_lib_) FindLibrary("python3.3", true);
				if (!shared_lib_) FindLibrary("python3.2", true);
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
					RESOLVE_PYTHON_SYMBOL(Py_SetProgramName);
					RESOLVE_PYTHON_SYMBOL(Py_GetProgramFullPath);
					RESOLVE_PYTHON_SYMBOL(PyImport_AppendInittab);
					RESOLVE_PYTHON_SYMBOL(PyType_Ready);
					RESOLVE_PYTHON_SYMBOL(PyCallable_Check);
					RESOLVE_PYTHON_SYMBOL(PyObject_GetAttrString);
					RESOLVE_PYTHON_SYMBOL(PyObject_HasAttrString);
					RESOLVE_PYTHON_SYMBOL(PyBytes_AsString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_AsASCIIString);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromString);
					RESOLVE_PYTHON_SYMBOL(PyLong_FromLong);
					RESOLVE_PYTHON_SYMBOL(PyLong_AsLongLong);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetDict);
					RESOLVE_PYTHON_SYMBOL(PyDict_New);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItemString);
					RESOLVE_PYTHON_SYMBOL(PyDict_SetItem);
					RESOLVE_PYTHON_SYMBOL(PyModule_GetState);
					RESOLVE_PYTHON_SYMBOL(PyState_FindModule);
					RESOLVE_PYTHON_SYMBOL(PyErr_Clear);
					RESOLVE_PYTHON_SYMBOL(PyErr_Fetch);
					RESOLVE_PYTHON_SYMBOL(PyImport_ImportModule);
					RESOLVE_PYTHON_SYMBOL(PyObject_CallObject);
					RESOLVE_PYTHON_SYMBOL(PyFrame_GetLineNumber);
					RESOLVE_PYTHON_SYMBOL(PyEval_RestoreThread);
					RESOLVE_PYTHON_SYMBOL(_Py_NegativeRefcount);
					RESOLVE_PYTHON_SYMBOL(_PyObject_New);
#ifdef DEBUG
					RESOLVE_PYTHON_SYMBOL(PyModule_Create2TraceRefs);
#else
					RESOLVE_PYTHON_SYMBOL(PyModule_Create2);
#endif
					RESOLVE_PYTHON_SYMBOL(PyModule_AddObject);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTuple);
					RESOLVE_PYTHON_SYMBOL(PyArg_ParseTupleAndKeywords);
					RESOLVE_PYTHON_SYMBOL(PyUnicode_FromFormat);
					RESOLVE_PYTHON_SYMBOL(Py_BuildValue);
#ifdef DEBUG
					RESOLVE_PYTHON_SYMBOL(_Py_Dealloc);
#endif
				}
			}
			_Py_NoneStruct.ob_refcnt = 1;
		};
		~SharedLibraryProxy() {};

		bool Py_LoadLibrary() { return (shared_lib_ != 0); };

#ifndef WIN32
		private:
			void FindLibrary(const char * szLibrary, bool bSimple = false)
			{
				if (bSimple)
				{
					// look in directories covered by ldconfig
					if (!shared_lib_)
					{
						std::string sLibrary = "lib";
						sLibrary += szLibrary;
						sLibrary += ".so";
						shared_lib_ = dlopen(sLibrary.c_str(), RTLD_LAZY | RTLD_GLOBAL);
//						if (shared_lib_) _log.Log(LOG_STATUS, "(%s) Loaded.", sLibrary.c_str());

					}
					// look in directories covered by ldconfig but 'm' variant
					if (!shared_lib_)
					{
						std::string sLibraryM = "lib";
						sLibraryM += szLibrary;
						sLibraryM += "m.so";
						shared_lib_ = dlopen(sLibraryM.c_str(), RTLD_LAZY | RTLD_GLOBAL);
//						if (shared_lib_) _log.Log(LOG_STATUS, "(%s) Loaded.", sLibraryM.c_str());
					}
					// look in /usr/lib directories
					if (!shared_lib_)
					{
						std::string	sLibraryDir = "/usr/lib/";
						sLibraryDir += szLibrary;
						sLibraryDir += "/";
						FindLibrary(sLibraryDir.c_str(), false);
					}
					// look in /usr/lib directories but 'm' variant
					if (!shared_lib_)
					{
						std::string	sLibraryMDir = "/usr/lib/";
						sLibraryMDir += szLibrary;
						sLibraryMDir += "m/";
						FindLibrary(sLibraryMDir.c_str(), false);
					}
				}
				else
				{
					DIR *lDir;
					struct dirent *ent;
					if ((lDir = opendir(szLibrary)) != NULL)
					{
						while (!shared_lib_ && (ent = readdir(lDir)) != NULL)
						{
							std::string filename = ent->d_name;
							if (dirent_is_directory(szLibrary, ent) && (filename.length() > 2))
							{
								std::string	newDir = szLibrary + filename + "/";
								FindLibrary(newDir.c_str());
							}
							else
							{
								if ((filename.length() > 12) &&
									(filename.compare(0, 11, "libpython3.") == 0) &&
									(filename.compare(filename.length() - 3, 3, ".so") == 0))
								{
									std::string sLibFile = szLibrary + filename;
									shared_lib_ = dlopen(sLibFile.c_str(), RTLD_LAZY | RTLD_GLOBAL);
//									_log.Log(LOG_STATUS, "(%s) Loaded.", sLibFile.c_str());
								}
							}
						}
						closedir(lDir);
					}
				}
			}
#endif
	};

	SharedLibraryProxy* pythonLib = new SharedLibraryProxy();

#define	Py_LoadLibrary			pythonLib->Py_LoadLibrary
#define	Py_GetVersion			pythonLib->Py_GetVersion
#define	Py_IsInitialized		pythonLib->Py_IsInitialized
#define	Py_Initialize			pythonLib->Py_Initialize
#define	Py_Finalize				pythonLib->Py_Finalize
#define	Py_NewInterpreter		pythonLib->Py_NewInterpreter
#define	Py_EndInterpreter		pythonLib->Py_EndInterpreter
#define	Py_SetPath				pythonLib->Py_SetPath
#define	Py_GetPath				pythonLib->Py_GetPath
#define	Py_SetProgramName		pythonLib->Py_SetProgramName
#define	Py_GetProgramFullPath	pythonLib->Py_GetProgramFullPath
#define	PyImport_AppendInittab	pythonLib->PyImport_AppendInittab
#define	PyType_Ready			pythonLib->PyType_Ready
#define	PyCallable_Check		pythonLib->PyCallable_Check
#define	PyObject_GetAttrString	pythonLib->PyObject_GetAttrString
#define	PyObject_HasAttrString	pythonLib->PyObject_HasAttrString
#define	PyBytes_AsString		pythonLib->PyBytes_AsString
// Python 3.2.3 has defined these as macros so we can't redefine them here
//#define PyUnicode_AsASCIIString pythonLib->PyUnicode_AsASCIIString
//#define PyUnicode_FromString	pythonLib->PyUnicode_FromString
//#define PyUnicode_FromFormat	pythonLib->PyUnicode_FromFormat
#define PyLong_FromLong			pythonLib->PyLong_FromLong
#define PyLong_AsLongLong		pythonLib->PyLong_AsLongLong
#define PyModule_GetDict		pythonLib->PyModule_GetDict
#define PyDict_New				pythonLib->PyDict_New
#define PyDict_SetItemString	pythonLib->PyDict_SetItemString
#define PyDict_SetItem			pythonLib->PyDict_SetItem
#define PyModule_GetState		pythonLib->PyModule_GetState
#define PyState_FindModule		pythonLib->PyState_FindModule
#define PyErr_Clear				pythonLib->PyErr_Clear
#define PyErr_Fetch				pythonLib->PyErr_Fetch
#define PyImport_ImportModule	pythonLib->PyImport_ImportModule
#define PyObject_CallObject		pythonLib->PyObject_CallObject
#define PyFrame_GetLineNumber	pythonLib->PyFrame_GetLineNumber
#define PyEval_RestoreThread	pythonLib->PyEval_RestoreThread
#define _Py_NegativeRefcount	pythonLib->_Py_NegativeRefcount
#define _PyObject_New			pythonLib->_PyObject_New
#define PyArg_ParseTuple		pythonLib->PyArg_ParseTuple
#define Py_BuildValue			pythonLib->Py_BuildValue
#ifdef DEBUG
#	define PyModule_Create2TraceRefs pythonLib->PyModule_Create2TraceRefs
#else
#	define PyModule_Create2		pythonLib->PyModule_Create2
#endif
#define PyModule_AddObject		pythonLib->PyModule_AddObject
#define PyArg_ParseTupleAndKeywords pythonLib->PyArg_ParseTupleAndKeywords

#ifdef DEBUG
#	define _Py_Dealloc			pythonLib->_Py_Dealloc
#endif

#define _Py_RefTotal			pythonLib->_Py_RefTotal
#define _Py_NoneStruct			pythonLib->_Py_NoneStruct
}