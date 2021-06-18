#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "../../main/Logger.h"
#include "../../main/SQLHelper.h"
#include "../../hardware/hardwaretypes.h"
#include "../../main/localtime_r.h"
#include "../../main/mainstructs.h"
#include "../../main/mainworker.h"
#include "../../main/EventSystem.h"
#include "../../notifications/NotificationHelper.h"
#include "Plugins.h"
#include "PythonObjectEx.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"
#include <datetime.h>

namespace Plugins {

	extern struct PyModuleDef DomoticzExModuleDef;
	extern void LogPythonException(CPlugin *pPlugin, const std::string &sHandler);
	extern void maptypename(const std::string &sTypeName, int &Type, int &SubType, int &SwitchType, std::string &sValue, PyObject *OptionsIn, PyObject *OptionsOut);

	void CDeviceEx_dealloc(CDeviceEx *self)
	{
		Py_XDECREF(self->DeviceID);
		Py_XDECREF(self->m_UnitDict);
		Py_TYPE(self)->tp_free((PyObject *)self);
	}

	PyObject *CDeviceEx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		CDeviceEx *self = (CDeviceEx *)type->tp_alloc(type, 0);

		try
		{
			if (self == nullptr)
			{
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else
			{
				self->DeviceID = PyUnicode_FromString("");
				if (self->DeviceID == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
			}
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}
		return (PyObject *)self;
	}

	int CDeviceEx_init(CDeviceEx *self, PyObject *args, PyObject *kwds)
	{
		const char *DeviceID = nullptr;

		try
		{
			PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "(%s) DomoticzEx module not found in interpreter.", __func__);
				return 0;
			}

			module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "(%s) unable to obtain module state.", __func__);
				return 0;
			}

			if (!pModState->pPlugin)
			{
				_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
				return 0;
			}

			// Normal case of DeviceEx creation by DeviceID
			static char *kwlist[] = { "DeviceID", nullptr };

			if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &DeviceID))
			{
				CPlugin *pPlugin = nullptr;
				if (pModState)
					pPlugin = pModState->pPlugin;
				pModState->pPlugin->Log(LOG_ERROR, R"(Expected: myVar = Domoticz.DeviceEx(DeviceID='xxxx'))");
				LogPythonException(pPlugin, __func__);
			}
			else
			{
				Py_DECREF(self->DeviceID);
				if (DeviceID)
				{
					self->DeviceID = PyUnicode_FromString(DeviceID);
				}
				self->m_UnitDict = (PyDictObject *)PyDict_New();
			}

			return true;
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}

		return false;
	}

	PyObject *CDeviceEx_refresh(CDeviceEx *self)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) DomoticzEx module not found in interpreter.", __func__);
			return 0;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) unable to obtain module state.", __func__);
			return 0;
		}

		if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
			return 0;
		}

		// Populate the unit dictionary if there are any
		std::string DeviceID = PyUnicode_AsUTF8(self->DeviceID);
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name, Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s')", pModState->pPlugin->m_HwdID, DeviceID.c_str());
		if (!result.empty())
		{

			PyType_Ready(&CUnitExType);
			// Create Unit objects and add the Units dictionary with Unit number as the key
			for (std::vector<std::vector<std::string>>::const_iterator itt = result.begin(); itt != result.end(); ++itt)
			{
				std::vector<std::string> sd = *itt;

				PyNewRef nrArgList = Py_BuildValue("(ssi)", sd[0].c_str(), DeviceID.c_str(), atoi(sd[1].c_str()));
				if (!nrArgList)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Building device argument list failed for key %s/%s.", sd[0].c_str(), sd[1].c_str());
					goto Error;
				}
				PyNewRef pUnit = PyObject_CallObject((PyObject *)pModState->pUnitClass, nrArgList);
				if (!pUnit)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Unit object creation failed for key %d.", atoi(sd[0].c_str()));
					goto Error;
				}

				// Add the object to the dictionary
				PyNewRef pKey = PyLong_FromLong(atoi(sd[1].c_str()));
				if (PyDict_SetItem((PyObject *)self->m_UnitDict, pKey, pUnit) == -1)
				{
					pModState->pPlugin->Log(LOG_ERROR, "Failed to add key '%s' to Unit dictionary.", std::string(pKey).c_str());
					goto Error;
				}

				// Force the object to refresh from the database
				PyNewRef pRefresh = PyObject_GetAttrString(pUnit, "Refresh");
				if (pRefresh && PyCallable_Check(pRefresh))
				{
					PyNewRef pReturnValue = PyObject_CallObject(pRefresh, NULL);
				}
				else
				{
					pModState->pPlugin->Log(LOG_ERROR, "Failed to refresh object '%s', method missing or not callable.", std::string(pKey).c_str());
				}
			}
		}
	Error:
		Py_RETURN_NONE;
	}

	PyObject *CDeviceEx_str(CDeviceEx *self)
	{
		PyObject *pRetVal = PyUnicode_FromFormat("DeviceID: '%U', Units: %d", self->DeviceID, PyDict_Size((PyObject*)self->m_UnitDict));
		return pRetVal;
	}

	void CUnitEx_dealloc(CUnitEx *self)
	{
		Py_XDECREF(self->Name);
		Py_XDECREF(self->LastUpdate);
		Py_XDECREF(self->Description);
		Py_XDECREF(self->sValue);
		PyDict_Clear(self->Options);
		Py_XDECREF(self->Options);
		Py_XDECREF(self->Color);
		Py_XDECREF(self->Parent);
		Py_TYPE(self)->tp_free((PyObject *)self);
	}

	PyObject *CUnitEx_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		CUnitEx *self = (CUnitEx *)type->tp_alloc(type, 0);

		try
		{
			if (self == nullptr)
			{
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else
			{
				self->Unit = 0;
				self->Type = 0;
				self->SubType = 0;
				self->SwitchType = 0;
				self->ID = -1;
				self->LastLevel = 0;
				self->Name = PyUnicode_FromString("");
				if (self->Name == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->LastUpdate = PyUnicode_FromString("");
				if (self->LastUpdate == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->nValue = 0;
				self->SignalLevel = 0;
				self->BatteryLevel = 0;
				self->sValue = PyUnicode_FromString("");
				if (self->sValue == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Image = 0;
				self->Options = (PyObject*)PyDict_New();
				self->Used = 0;
				self->Description = PyUnicode_FromString("");
				if (self->Description == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Color = PyUnicode_FromString("");
				if (self->Color == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Parent = nullptr;
			}
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}
		return (PyObject *)self;
	}

	int CUnitEx_init(CUnitEx *self, PyObject *args, PyObject *kwds)
	{
		char *Name = nullptr;
		char *DeviceID = nullptr;
		int Unit = -1;
		char *TypeName = nullptr;
		int Type = -1;
		int SubType = -1;
		int SwitchType = -1;
		int Image = -1;
		PyObject *Options = nullptr;
		int Used = -1;
		char *Description = nullptr;
		static char *kwlist[] = { "Name", "DeviceID", "Unit", "TypeName", "Type", "Subtype", "Switchtype", "Image", "Options", "Used", "Description", nullptr };

		try
		{
			PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "(%s) Domoticz module not found in interpreter.", __func__);
				return 0;
			}

			module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
				return 0;
			}

			if (!pModState->pPlugin)
			{
				_log.Log(LOG_ERROR, "(%s) Illegal operation, Plugin has not started yet.", __func__);
				return 0;
			}

				// otherwise a new Unit is being created
			if (PyArg_ParseTupleAndKeywords(args, kwds, "ssi|siiiiOis", kwlist, &Name, &DeviceID, &Unit, &TypeName, &Type, &SubType, &SwitchType, &Image, &Options, &Used, &Description))
			{
				char szID[40];
				if (Name)
				{
					Py_DECREF(self->Name);
					self->Name = PyUnicode_FromString(Name);
				}
				if (Description)
				{
					Py_DECREF(self->Description);
					self->Description = PyUnicode_FromString(Description);
				}
				if ((Unit > 0) && (Unit < 256))
				{
					self->Unit = Unit;
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Illegal Unit number (%d), valid values range from 1 to 255.", __func__, Unit);
					return 0;
				}
				if (!DeviceID)
				{
					sprintf(szID, "%04X%04X", pModState->pPlugin->m_HwdID, self->Unit); // Generate a Device ID if one was not supplied
					DeviceID = &szID[0];
				}
				self->Parent = PyDict_GetItemString((PyObject *)pModState->pPlugin->m_DeviceDict, DeviceID);
				if (self->Parent)
				{
					Py_INCREF(self->Parent);
				}
				else
				{
					// Create a temporary one
					PyType_Ready(pModState->pDeviceClass);
					PyNewRef nrArgList = Py_BuildValue("(s)", DeviceID);
					if (!nrArgList)
					{
						_log.Log(LOG_ERROR, "Building device argument list failed for key '%s'.", DeviceID);
						return 0;
					}
					self->Parent = PyObject_CallObject((PyObject *)pModState->pDeviceClass, nrArgList);
					if (!self->Parent)
					{
						_log.Log(LOG_ERROR, "Device object creation failed for key '%s'.", DeviceID);
						return 0;
					}
				}
				if (TypeName)
				{
					std::string sValue;
					maptypename(std::string(TypeName), self->Type, self->SubType, self->SwitchType, sValue, Options, self->Options);
					Py_DECREF(self->sValue);
					self->sValue = PyUnicode_FromString(sValue.c_str());
				}
				if ((Type != -1) && Type)
					self->Type = Type;
				if ((SubType != -1) && SubType)
					self->SubType = SubType;
				if (SwitchType != -1)
					self->SwitchType = SwitchType;
				if (Image != -1)
					self->Image = Image;
				if (Used == 1)
					self->Used = Used;
				if (Options && PyDict_Check(Options) && PyDict_Size(Options) > 0)
				{
					PyObject *pKey, *pValue;
					Py_ssize_t pos = 0;
					PyDict_Clear(self->Options);
					while (PyDict_Next(Options, &pos, &pKey, &pValue))
					{
						if (PyUnicode_Check(pValue))
						{
							PyNewRef	pKeyDict = PyUnicode_FromKindAndData(PyUnicode_KIND(pKey), PyUnicode_DATA(pKey), PyUnicode_GET_LENGTH(pKey));
							PyNewRef	pValueDict = PyUnicode_FromKindAndData(PyUnicode_KIND(pValue), PyUnicode_DATA(pValue), PyUnicode_GET_LENGTH(pValue));
							if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
							{
								_log.Log(LOG_ERROR, "(%s) Failed to initialize Options dictionary for Hardware/Unit combination (%d:%d).",
										pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit);
								break;
							}
						}
						else
						{
							_log.Log(
								LOG_ERROR,
								R"((%s) Failed to initialize Options dictionary for Hardware/Unit combination (%d:%d): Only "string" type dictionary entries supported, but entry has type "%s")",
								pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit, pValue->ob_type->tp_name);
						}
					}
				}
			}
			else
			{
				CPlugin *pPlugin = nullptr;
				if (pModState)
				{
					pPlugin = pModState->pPlugin;
					_log.Log(LOG_ERROR, R"(Expected: myVar = DomoticzEx.Unit(Name="myDevice", DeviceID="", Unit=0, TypeName="", Type=0, Subtype=0, Switchtype=0, Image=0, Options={}, Used=1))");
					LogPythonException(pPlugin, __func__);
				}
			}
		}
		catch (std::exception *e)
		{
			_log.Log(LOG_ERROR, "%s: Execption thrown: %s", __func__, e->what());
		}
		catch (...)
		{
			_log.Log(LOG_ERROR, "%s: Unknown execption thrown", __func__);
		}

		return 0;
	}

	PyObject *CUnitEx_refresh(CUnitEx *self)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) DomoticzEx module not found in interpreter.", __func__);
			return 0;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) unable to obtain module state.", __func__);
			return 0;
		}

		if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
			return 0;
		}

		if ((pModState->pPlugin) && (pModState->pPlugin->m_HwdID != -1) && (self->Unit != -1))
		{
			CDeviceEx *pDevice = (CDeviceEx*)self->Parent;
			std::string sDevice = PyBorrowedRef(pDevice->DeviceID);
			// load associated devices to make them available to python
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, Type, SubType, SwitchType, LastLevel, CustomImage, SignalLevel, BatteryLevel, LastUpdate, Options, "
						  "Description, Color, Used FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d) ORDER BY Unit ASC",
						  pModState->pPlugin->m_HwdID, sDevice.c_str(), self->Unit);
			if (!result.empty())
			{
				for (const auto &sd : result)
				{
					self->Unit = atoi(sd[0].c_str());
					self->ID = atoi(sd[1].c_str());
					Py_XDECREF(self->Name);
					self->Name = PyUnicode_FromString(sd[2].c_str());
					self->nValue = atoi(sd[3].c_str());
					Py_XDECREF(self->sValue);
					self->sValue = PyUnicode_FromString(sd[4].c_str());
					self->Type = atoi(sd[5].c_str());
					self->SubType = atoi(sd[6].c_str());
					self->SwitchType = atoi(sd[7].c_str());
					self->LastLevel = atoi(sd[8].c_str());
					self->Image = atoi(sd[9].c_str());
					self->SignalLevel = atoi(sd[10].c_str());
					self->BatteryLevel = atoi(sd[11].c_str());
					Py_XDECREF(self->LastUpdate);
					self->LastUpdate = PyUnicode_FromString(sd[12].c_str());
					PyDict_Clear(self->Options);
					if (!sd[13].empty())
					{
						if (self->SubType == sTypeCustom)
						{
							PyDict_SetItemString(self->Options, "Custom", PyUnicode_FromString(sd[13].c_str()));
						}
						else
						{
							std::map<std::string, std::string> mpOptions;
							Py_BEGIN_ALLOW_THREADS mpOptions = m_sql.BuildDeviceOptions(sd[13], true);
							Py_END_ALLOW_THREADS for (const auto &opt : mpOptions)
							{
								PyNewRef pKeyDict = PyUnicode_FromString(opt.first.c_str());
								PyNewRef pValueDict = PyUnicode_FromString(opt.second.c_str());
								if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
								{
									_log.Log(LOG_ERROR, "(%s) Failed to refresh Options dictionary for Hardware/Unit combination (%d:%d).",
										 pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit);
									break;
								}
							}
						}
					}
					Py_XDECREF(self->Description);
					self->Description = PyUnicode_FromString(sd[14].c_str());
					Py_XDECREF(self->Color);
					self->Color = PyUnicode_FromString(_tColor(std::string(sd[15])).toJSONString().c_str()); // Parse the color to detect incorrectly formatted color data
					self->Used = atoi(sd[16].c_str());
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device refresh failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject *CUnitEx_insert(CUnitEx *self)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) Domoticz module not found in interpreter.", __func__);
			Py_RETURN_NONE;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (pModState->pPlugin)
		{
			std::string sName = PyUnicode_AsUTF8(self->Name);
			CDeviceEx *pDevice = (CDeviceEx *)self->Parent;
			std::string sDeviceID = PyUnicode_AsUTF8(pDevice->DeviceID);
			if (self->ID == -1)
			{
				if (pModState->pPlugin->m_bDebug & PDM_DEVICE)
				{
					pModState->pPlugin->Log(LOG_NORM, "(%s) Creating Unit '%s'.", pModState->pPlugin->m_Name.c_str(), sName.c_str());
				}

				if (!m_sql.m_bAcceptNewHardware)
				{
					pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit creation failed, Domoticz settings prevent accepting new devices.", pModState->pPlugin->m_Name.c_str());
				}
				else
				{
					std::vector<std::vector<std::string>> result;
					result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
					if (result.empty())
					{
						std::string sValue = PyUnicode_AsUTF8(self->sValue);
						std::string sColor = _tColor(std::string(PyUnicode_AsUTF8(self->Color))).toJSONString(); // Parse the color to detect incorrectly formatted color data
						std::string sLongName = sName;
						std::string sDescription = PyUnicode_AsUTF8(self->Description);
						if ((self->SubType == sTypeCustom) && (PyDict_Size(self->Options) > 0))
						{
							PyBorrowedRef pValueDict = PyDict_GetItemString(self->Options, "Custom");
							std::string sOptionValue;
							if (!pValueDict)
								sOptionValue = "";
							else
								sOptionValue = PyUnicode_AsUTF8(pValueDict);

							m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, "
									 "nValue, sValue, CustomImage, Description, Color, Options) "
									 "VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q', '%q', '%q')",
									 pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used,
									 sLongName.c_str(), sValue.c_str(), self->Image, sDescription.c_str(), sColor.c_str(), sOptionValue.c_str());
						}
						else
						{
							m_sql.safe_query("INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, "
									 "nValue, sValue, CustomImage, Description, Color) "
									 "VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q', '%q')",
									 pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used,
									 sLongName.c_str(), sValue.c_str(), self->Image, sDescription.c_str(), sColor.c_str());
						}

						result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
						if (!result.empty())
						{
							self->ID = atoi(result[0][0].c_str());

							// DeviceStatus successfully created, now set the options when supplied
							if ((self->SubType != sTypeCustom) && (PyDict_Size(self->Options) > 0))
							{
								PyBorrowedRef	pKeyDict, pValueDict;
								Py_ssize_t pos = 0;
								std::map<std::string, std::string> mpOptions;
								while (PyDict_Next(self->Options, &pos, &pKeyDict, &pValueDict))
								{
									std::string sOptionName = pKeyDict;
									std::string sOptionValue = pValueDict;
									mpOptions.insert(std::pair<std::string, std::string>(sOptionName, sOptionValue));
								}
								m_sql.SetDeviceOptions(self->ID, mpOptions);
							}

							// Check the parent device is in the plugin dictionary (can happen if this Unit has just been created)
							if (!PyDict_Contains((PyObject *)pModState->pPlugin->m_DeviceDict, pDevice->DeviceID))
							{
								if (PyDict_SetItem((PyObject *)pModState->pPlugin->m_DeviceDict, pDevice->DeviceID, self->Parent) == -1)
								{
									pModState->pPlugin->Log(LOG_ERROR, "(%s) failed to add key '%s' to device dictionary.", pModState->pPlugin->m_PluginKey.c_str(),
												sDeviceID.c_str());
								}
							}
							else
							{
								// otherwise update the parent if required
								PyBorrowedRef pParent = PyDict_GetItem((PyObject *)pModState->pPlugin->m_DeviceDict, pDevice->DeviceID);
								if (self->Parent != pParent)
								{
									Py_DECREF(self->Parent);
									self->Parent = pParent;
								}
							}

							// Now insert the Unit in the Parent's Units diction
							PyNewRef pKey = PyLong_FromLong(self->Unit);
							if (PyDict_SetItem((PyObject *)((CDeviceEx*)self->Parent)->m_UnitDict, pKey, (PyObject *)self) == -1)
							{
								pModState->pPlugin->Log(LOG_ERROR, "(%s) failed to add unit '%d' to device dictionary.", pModState->pPlugin->m_Name.c_str(),
											self->Unit);
								Py_RETURN_NONE;
							}

							// Refresh DeviceStatus data to ensure it is usable straight away
							PyNewRef	pRetVal = CUnitEx_refresh(self);
						}
						else
						{
							pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit creation failed, Hardware/DeviceID/Unit combination (%d/%s/%d) not found in Domoticz.",
										pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
						}
					}
					else
					{
						pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit creation failed, Hardware/Unit combination (%d:%d) already exists in Domoticz.",
									pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit);
					}
				}
			}
			else
			{
				pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit creation failed, '%s' already exists in Domoticz with Device ID '%d'.", pModState->pPlugin->m_Name.c_str(), sName.c_str(), self->ID);
			}
		}
		else
		{
			pModState->pPlugin->Log(LOG_ERROR, "Unit creation failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject *CUnitEx_update(CUnitEx *self, PyObject *args, PyObject *kwds)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) Domoticz module not found in interpreter.", __func__);
			Py_RETURN_NONE;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (pModState->pPlugin)
		{
			pModState->pPlugin->SetHeartbeatReceived();

			char *TypeName = nullptr;
			int SuppressTriggers = false;

			static char *kwlist[] = { "TypeName", "SuppressTriggers", nullptr };

			// Try to extract parameters needed to update device settings
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sp", kwlist, &SuppressTriggers))
			{
				pModState->pPlugin->Log(LOG_ERROR,
					 "(%s) Failed to parse parameters: 'SuppressTriggers' expected.", __func__);
				LogPythonException(pModState->pPlugin, __func__);
				Py_INCREF(Py_None);
				return Py_None;
			}

			CDeviceEx *pDevice = (CDeviceEx *)self->Parent;
			std::string sDeviceID = PyUnicode_AsUTF8(pDevice->DeviceID);
			std::string sID = std::to_string(self->ID);

			// Build representations of changeable fields (these may not be of the right type so be defensive)
			std::string sName = PyBorrowedRef(self->Name);
			std::string sDescription = PyBorrowedRef(self->Description);
			std::string sValue = PyBorrowedRef(self->sValue);
			std::string sColor = PyBorrowedRef(self->Color);
			sColor = _tColor(sColor).toJSONString();
			int nValue = self->nValue;
			int iType = self->Type;
			int iSubType = self->SubType;
			int iSwitchType = self->SwitchType;
			PyBorrowedRef pOptionsDict = self->Options;

			// TypeName change - actually derives new Type, SubType and SwitchType values
			if (TypeName)
			{
				std::string stdsValue;
				maptypename(std::string(TypeName), iType, iSubType, iSwitchType, stdsValue, pOptionsDict, pOptionsDict);

				// Reset nValue and sValue when changing device types
				nValue = 0;
				sValue = stdsValue;
			}

			// Options provided, assume change
			std::string sOptionValue;
			if (pOptionsDict && PyDict_Check(pOptionsDict))
			{
				if (self->SubType != sTypeCustom)
				{
					PyBorrowedRef	pKeyDict, pValueDict;
					Py_ssize_t pos = 0;
					std::map<std::string, std::string> mpOptions;
					while (PyDict_Next(self->Options, &pos, &pKeyDict, &pValueDict))
					{
						std::string sOptionName = pKeyDict;
						std::string sOptionValue = pValueDict;
						mpOptions.insert(std::pair<std::string, std::string>(sOptionName, sOptionValue));
					}
					sOptionValue = m_sql.FormatDeviceOptions(mpOptions);
				}
				else
				{
					PyBorrowedRef pValue = PyDict_GetItemString(pOptionsDict, "Custom");
					if (pValue)
					{
						sOptionValue = (std::string)pValue;
					}
				}
			}

			// Do an atomic update of non-key fields
			Py_BEGIN_ALLOW_THREADS
			m_sql.safe_query("UPDATE DeviceStatus SET Name='%s', Description='%s', Used=%d, Type=%d, SubType=%d, SwitchType=%d, CustomImage=%d, Color='%s', SignalLevel=%d, BatteryLevel=%d, Options='%s', LastUpdate='%s' WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", 
								sName.c_str(), sDescription.c_str(), self->Used, iType, iSubType, iSwitchType, self->Image, sColor.c_str(), self->SignalLevel,
								self->BatteryLevel, sOptionValue.c_str(), TimeToString(nullptr, TF_DateTime).c_str(), pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
			Py_END_ALLOW_THREADS

			// Suppress Triggers updates non-key fields only (specifically NOT nValue or sValue)
			if (!SuppressTriggers)
			{
				uint64_t DevRowIdx;
				if (pModState->pPlugin->m_bDebug & PDM_DEVICE)
				{
					pModState->pPlugin->Log(LOG_NORM, "(%s) Updating device from %d:'%s' to have values %d:'%s'.", sName.c_str(), self->nValue, PyUnicode_AsUTF8(self->sValue), nValue, sValue.c_str());
				}
				Py_BEGIN_ALLOW_THREADS 
				DevRowIdx = m_sql.UpdateValue(pModState->pPlugin->m_HwdID, sDeviceID.c_str(), (const unsigned char)self->Unit, (const unsigned char)iType, (const unsigned char)iSubType, self->SignalLevel, self->BatteryLevel, nValue, sValue.c_str(), sName, true);
				Py_END_ALLOW_THREADS

				// if this is an internal Security Panel then there are some extra updates required if state has changed
				if ((self->Type == pTypeSecurity1) && (self->SubType == sTypeDomoticzSecurity) && (self->nValue != nValue))
				{
					switch (nValue)
					{
						case sStatusArmHome:
						case sStatusArmHomeDelayed:
							Py_BEGIN_ALLOW_THREADS m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDHOME);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDHOME);
							Py_END_ALLOW_THREADS break;
						case sStatusArmAway:
						case sStatusArmAwayDelayed:
							Py_BEGIN_ALLOW_THREADS m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDAWAY);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDAWAY);
							Py_END_ALLOW_THREADS break;
						case sStatusDisarm:
						case sStatusNormal:
						case sStatusNormalDelayed:
						case sStatusNormalTamper:
						case sStatusNormalDelayedTamper:
							Py_BEGIN_ALLOW_THREADS m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_DISARMED);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_DISARMED);
							Py_END_ALLOW_THREADS break;
					}
				}

				// Notify MQTT and various push mechanisms and notifications
				Py_BEGIN_ALLOW_THREADS
				m_mainworker.sOnDeviceReceived(pModState->pPlugin->m_HwdID, self->ID, pModState->pPlugin->m_Name, NULL);
				m_notifications.CheckAndHandleNotification(DevRowIdx, pModState->pPlugin->m_HwdID, sDeviceID, sName, self->Unit, iType, iSubType, nValue, sValue);
				m_mainworker.CheckSceneCode(DevRowIdx, (const unsigned char)self->Type, (const unsigned char)self->SubType, nValue, sValue.c_str(), "Python");
				Py_END_ALLOW_THREADS
			}

			PyNewRef	pRetVal = CUnitEx_refresh(self);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device update failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject *CUnitEx_delete(CUnitEx *self)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) Domoticz module not found in interpreter.", __func__);
			Py_RETURN_NONE;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (!pModState->pPlugin)
		{
			std::string sName = PyUnicode_AsUTF8(self->Name);
			if (self->ID != -1)
			{
				if (pModState->pPlugin->m_bDebug & PDM_DEVICE)
				{
					pModState->pPlugin->Log(LOG_NORM, "(%s) Deleting unit '%s'.", pModState->pPlugin->m_Name.c_str(), sName.c_str());
				}

				// Make sure the entry to delete exists and is for the correct hardware
				std::vector<std::vector<std::string>> result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (ID==%d)", pModState->pPlugin->m_HwdID, self->ID);
				if (!result.empty())
				{
					m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (ID==%d)", pModState->pPlugin->m_HwdID, self->ID);
				}
				else
				{
					pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit deletion failed, Hardware %d does not have a Unit with ID %d in the database.",
								pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->ID);
				}
			}
			else
			{
				pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit deletion failed, '%s' does not represent a device in Domoticz.", pModState->pPlugin->m_Name.c_str(), sName.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Unit deletion failed, UnitEx object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject *CUnitEx_touch(CUnitEx *self)
	{
		PyBorrowedRef pModule = PyState_FindModule(&DomoticzExModuleDef);
		if (!pModule)
		{
			_log.Log(LOG_ERROR, "(%s) DomoticzEx module not found in interpreter.", __func__);
			return 0;
		}

		module_state *pModState = ((struct module_state *)PyModule_GetState(pModule));
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) unable to obtain module state.", __func__);
			return 0;
		}

		if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
			return 0;
		}

		Py_BEGIN_ALLOW_THREADS if ((pModState->pPlugin) && (self->ID != -1))
		{
			pModState->pPlugin->SetHeartbeatReceived();
			std::string sID = std::to_string(self->ID);
			m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%s' WHERE (ID == %s )", TimeToString(nullptr, TF_DateTime).c_str(), sID.c_str());
		}
		else
		{
			pModState->pPlugin->Log(LOG_ERROR, "Unit touch failed, UnitEx object is not associated with a plugin.");
		}
		Py_END_ALLOW_THREADS
		
		return CUnitEx_refresh(self);
	}

	PyObject *CUnitEx_str(CUnitEx *self)
	{
		PyObject *pRetVal = PyUnicode_FromFormat("Unit: %d, Name: '%U', nValue: %d, sValue: '%U'", self->Unit, self->Name, self->nValue, self->sValue);
		return pRetVal;
	}
} // namespace Plugins
#endif
