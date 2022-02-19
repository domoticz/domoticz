#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2021
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
			Py_RETURN_NONE;
		}

		// Populate the unit dictionary if there are any
		std::string DeviceID = PyBorrowedRef(self->DeviceID);
		std::vector<std::vector<std::string>> result;
		result = m_sql.safe_query("SELECT Name, Unit FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s')", pModState->pPlugin->m_HwdID, DeviceID.c_str());
		if (!result.empty())
		{

			PyType_Ready(&CUnitExType);
			// Create Unit objects and add the Units dictionary with Unit number as the key
			for (auto itt = result.begin(); itt != result.end(); ++itt)
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

	bool CDeviceEx::isInstance(PyObject *pObject)
	{
		if (pObject)
		{
			PyBorrowedRef brModule = PyState_FindModule(&DomoticzExModuleDef);
			if (brModule)
			{
				module_state* pModState = ((struct module_state*)PyModule_GetState(brModule));
				if (pModState)
				{
					int isDevice = PyObject_IsInstance(pObject, (PyObject*)pModState->pDeviceClass);
					if (isDevice == -1)
					{
						_log.Log(LOG_ERROR, "%s: Error determining type of Python object", __func__);
						if (PyErr_Occurred())
						{
							PyErr_Clear();
						}
					}
					else if (isDevice)
					{
						return true;
					}
				}
			}
		}

		return false;
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
				self->SignalLevel = 12;
				self->BatteryLevel = 255;
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
					_log.Log(LOG_ERROR, R"(Expected: myVar = DomoticzEx.Unit(Name="myDevice", DeviceID="", Unit=0, TypeName="", Type=0, Subtype=0, Switchtype=0, Image=0, Options={}, Used=1, Description=""))");
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (!pModState->pPlugin)
		{
			_log.Log(LOG_ERROR, "(%s) illegal operation, Plugin has not started yet.", __func__);
			Py_RETURN_NONE;
		}

		if ((pModState->pPlugin) && (pModState->pPlugin->m_HwdID != -1) && (self->Unit != -1))
		{
			CDeviceEx *pDevice = (CDeviceEx*)self->Parent;
			std::string sDevice = PyBorrowedRef(pDevice->DeviceID);
			// load associated devices to make them available to python
			std::vector<std::vector<std::string>> result;
			result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, Type, SubType, SwitchType, LastLevel, CustomImage, SignalLevel, BatteryLevel, LastUpdate, Options, "
						  "Description, Color, Used, AddjValue, AddjMulti FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d) ORDER BY Unit ASC",
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
					self->Adjustment = static_cast<float>(atof(sd[17].c_str()));
					self->Multiplier = static_cast<float>(atof(sd[18].c_str()));
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (pModState->pPlugin)
		{
			std::string sName = PyBorrowedRef(self->Name);
			if (!(CDeviceEx*)self->Parent)
			{
				_log.Log(LOG_ERROR, "(%s) Unit is not associated with a Device.", __func__);
				Py_RETURN_NONE;
			}
			CDeviceEx *pDevice = (CDeviceEx *)self->Parent;
			std::string sDeviceID = PyBorrowedRef(pDevice->DeviceID);
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
						std::string sValue = PyBorrowedRef(self->sValue);
						std::string sColor = _tColor(std::string(PyBorrowedRef(self->Color))).toJSONString(); // Parse the color to detect incorrectly formatted color data
						std::string sLongName = sName;
						std::string sDescription = PyBorrowedRef(self->Description);
						std::string sOptionValue = "";

						// Support weird legacy 'custom' options
						//if ((self->SubType == sTypeCustom) && (PyDict_Size(self->Options) > 0))
						//{
						//	PyBorrowedRef pValueDict = PyDict_GetItemString(self->Options, "Custom");
						//	if (pValueDict)
						//		sOptionValue = (std::string)pValueDict;
						//}

						if (PyDict_Size(self->Options) > 0)
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
								PyBorrowedRef pValue = PyDict_GetItemString(self->Options, "Custom");
								if (pValue)
								{
									sOptionValue = (std::string)pValue;
								}
							}
						}


						std::string sSQL = "INSERT INTO DeviceStatus "
								   "(HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Description, Color, Options, LastUpdate) "
								   "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

						std::vector<std::string> vValues;
						// Keys
						vValues.push_back(std::to_string(pModState->pPlugin->m_HwdID));
						vValues.push_back(sDeviceID);
						vValues.push_back(std::to_string(self->Unit));
						// Values
						vValues.push_back(std::to_string(self->Type));
						vValues.push_back(std::to_string(self->SubType));
						vValues.push_back(std::to_string(self->SwitchType));
						vValues.push_back(std::to_string(self->Used));
						vValues.push_back(std::to_string(self->SignalLevel));
						vValues.push_back(std::to_string(self->BatteryLevel));
						vValues.push_back(sName);
						vValues.push_back(std::to_string(self->nValue));
						vValues.push_back(sValue);
						vValues.push_back(std::to_string(self->Image));
						vValues.push_back(sDescription);
						vValues.push_back(sColor);
						vValues.push_back(sOptionValue);
						vValues.push_back(TimeToString(nullptr, TF_DateTime));

						// Handle any data we get back (this method allows for any special characters in the strings such as ' and ")
						if (!m_sql.execute_sql(sSQL, &vValues, true))
						{
							pModState->pPlugin->Log(LOG_ERROR, "Creation of 'UnitEx' failed to insert a DeviceStatus record for key %d/%s/%d", 
										pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
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
						pModState->pPlugin->Log(LOG_ERROR, "(%s) Unit creation failed, Hardware/DeviceID/Unit combination (%d/%s/%d) already exists in Domoticz.",
									pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (pModState->pPlugin)
		{
			pModState->pPlugin->SetHeartbeatReceived();

			char *TypeName = nullptr;
			int bWriteLog = false;

			static char *kwlist[] = { "Log", "TypeName", nullptr };

			// Try to extract parameters needed to update device settings
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "|ps", kwlist, &bWriteLog, &TypeName))
			{
				pModState->pPlugin->Log(LOG_ERROR, "(%s) Failed to parse parameters: 'Log' and/or 'TypeName' expected.", __func__);
				LogPythonException(pModState->pPlugin, __func__);
				Py_RETURN_NONE;
			}

			CDeviceEx *pDevice = (CDeviceEx *)self->Parent;
			std::string sDeviceID = PyBorrowedRef(pDevice->DeviceID);
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

			// Need to look up current nValue and sValue and only do triggers if one has changed
			std::vector<std::vector<std::string>> result;

			result = m_sql.safe_query("SELECT ID, StrParam1, StrParam2 FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%s') AND (Unit==%d)", pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
			if (result.empty())
			{
				pModState->pPlugin->Log(LOG_ERROR, "Update to 'UnitEx' failed, row not found in database for key %d/%s/%d", pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
				Py_RETURN_NONE;
			}

			uint64_t	DevRowIdx = std::stoull(result[0][0]);
			std::string	sOnAction = result[0][1];
			std::string	sOffAction = result[0][2];
			int	iRows = 0;

			// Do an atomic update (do not change this to individual field updates!!!!!!!)
			Py_BEGIN_ALLOW_THREADS
			std::string sSQL = "UPDATE DeviceStatus "
							   "SET Name=?, Description=?, Used=?, Type=?, SubType=?, SwitchType=?, nValue=?, sValue=?, LastLevel=?, CustomImage=?, Color=?, SignalLevel=?, BatteryLevel=?, Options=?, LastUpdate=? "
							   "WHERE (HardwareID==?) AND (DeviceID==?) AND (Unit==?);";
			std::vector<std::string> vValues;
			vValues.push_back(sName);
			vValues.push_back(sDescription);
			vValues.push_back(std::to_string(self->Used));
			vValues.push_back(std::to_string(iType));
			vValues.push_back(std::to_string(iSubType));
			vValues.push_back(std::to_string(iSwitchType));
			vValues.push_back(std::to_string(nValue));
			vValues.push_back(sValue);
			vValues.push_back(std::to_string(self->LastLevel));
			vValues.push_back(std::to_string(self->Image));
			vValues.push_back(sColor);
			vValues.push_back(std::to_string(self->SignalLevel));
			vValues.push_back(std::to_string(self->BatteryLevel));
			vValues.push_back(sOptionValue);
			vValues.push_back(TimeToString(nullptr, TF_DateTime));
			// Keys
			vValues.push_back(std::to_string(pModState->pPlugin->m_HwdID));
			vValues.push_back(sDeviceID);
			vValues.push_back(std::to_string(self->Unit));

			// Handle any data we get back (this method allows for any special characters in the strings such as ' and ")
			iRows = m_sql.execute_sql(sSQL, &vValues, true);
			Py_END_ALLOW_THREADS

			if (!iRows)
			{
				pModState->pPlugin->Log(LOG_ERROR, "Update to 'UnitEx' failed to update any DeviceStatus records for key %d/%s/%d", pModState->pPlugin->m_HwdID, sDeviceID.c_str(), self->Unit);
				Py_RETURN_NONE;
			}

			// Only trigger notifications if a used value is changed
			if (self->Used)
			{
				// if this is an internal Security Panel then there are some extra updates required if state has changed
				if ((self->Type == pTypeSecurity1) && (self->SubType == sTypeDomoticzSecurity) && (self->nValue != nValue))
				{
					Py_BEGIN_ALLOW_THREADS
					switch (nValue)
					{
						case sStatusArmHome:
						case sStatusArmHomeDelayed:
							m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDHOME);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDHOME, "Python");
							break;
						case sStatusArmAway:
						case sStatusArmAwayDelayed:
							m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDAWAY);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDAWAY, "Python");
							break;
						case sStatusDisarm:
						case sStatusNormal:
						case sStatusNormalDelayed:
						case sStatusNormalTamper:
						case sStatusNormalDelayedTamper:
							m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_DISARMED);
							m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_DISARMED, "Python");
							break;
					}
					Py_END_ALLOW_THREADS
				}

				// Notify Event system, MQTT and various push mechanisms and notifications
				Py_BEGIN_ALLOW_THREADS
				m_mainworker.m_eventsystem.ProcessDevice(pModState->pPlugin->m_HwdID, DevRowIdx, self->Unit, iType, iSubType, self->SignalLevel, self->BatteryLevel, nValue, sValue.c_str());

				// Standard on/off action handling  If a device has these then make sure they are handled.
				if (self->SwitchType == STYPE_Selector)
				{
					sOnAction = GetSelectorSwitchLevelAction(m_sql.BuildDeviceOptions(sOptionValue, true), atoi(sValue.c_str()));
					sOffAction = GetSelectorSwitchLevelAction(m_sql.BuildDeviceOptions(sOptionValue, true), 0);
				}
				if (sOnAction.length() || sOffAction.length())
				{
					// Handle On & Off actions if they are defined (HandleOnOffAction just returns if they are blank)
					m_sql.HandleOnOffAction(nValue, sOnAction, sOffAction);
				}

				m_mainworker.sOnDeviceReceived(pModState->pPlugin->m_HwdID, self->ID, pModState->pPlugin->m_Name, NULL);

				// Notifications
				if (!IsLightOrSwitch(iType, iSubType))
				{
					m_notifications.CheckAndHandleNotification(DevRowIdx, pModState->pPlugin->m_HwdID, sDeviceID, sName, self->Unit, iType, iSubType, nValue, sValue);
				}
				else
				{
					std::string lstatus;
					int llevel;
					bool bHaveDimmer;
					int maxDimLevel;
					bool bHaveGroupCmd;
					GetLightStatus(iType, iSubType, (_eSwitchType)iSwitchType, nValue, sValue, lstatus, llevel, bHaveDimmer, maxDimLevel, bHaveGroupCmd);
					if (self->SwitchType == STYPE_Selector)
						m_notifications.CheckAndHandleSwitchNotification(DevRowIdx, sName, (IsLightSwitchOn(lstatus)) ? NTYPE_SWITCH_ON : NTYPE_SWITCH_OFF, llevel);
					else
						m_notifications.CheckAndHandleSwitchNotification(DevRowIdx, sName, (IsLightSwitchOn(lstatus)) ? NTYPE_SWITCH_ON : NTYPE_SWITCH_OFF);
				}
				m_mainworker.CheckSceneCode(DevRowIdx, (const unsigned char)self->Type, (const unsigned char)self->SubType, nValue, sValue.c_str(), "Python");

				// Write a log entry if requested
				if (bWriteLog)
				{
					std::string sSQL = "INSERT INTO LightingLog (DeviceRowID, nValue, sValue, Date) VALUES (?,?,?,?)";
					std::vector<std::string> vValues;
					vValues.push_back(result[0][0]);
					vValues.push_back(std::to_string(nValue));
					vValues.push_back(sValue);
					vValues.push_back(TimeToString(nullptr, TF_DateTime));
					if (!m_sql.execute_sql(sSQL, &vValues, true))
					{
						pModState->pPlugin->Log(LOG_ERROR, "%s: Insert into 'LightingLog' failed ", __func__);
					}
				}
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
		}

		if (!pModState->pPlugin)
		{
			std::string sName = PyBorrowedRef(self->Name);
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
		module_state *pModState = CPlugin::FindModule();
		if (!pModState)
		{
			_log.Log(LOG_ERROR, "(%s) Unable to obtain module state.", __func__);
			Py_RETURN_NONE;
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
		PyObject *pRetVal = PyUnicode_FromFormat("Unit: %d, Name: '%U', nValue: %d, sValue: '%U', LastUpdate: %U", self->Unit, self->Name, self->nValue, self->sValue, self->LastUpdate);
		return pRetVal;
	}

	bool CUnitEx::isInstance(PyObject *pObject)
	{
		if (pObject)
		{
			PyBorrowedRef brModule = PyState_FindModule(&DomoticzExModuleDef);
			if (brModule)
			{
				module_state* pModState = ((struct module_state*)PyModule_GetState(brModule));
				if (pModState)
				{
					int isUnit = PyObject_IsInstance(pObject, (PyObject*)pModState->pUnitClass);
					if (isUnit == -1)
					{
						_log.Log(LOG_ERROR, "%s: Error determining type of Python object", __func__);
						if (PyErr_Occurred())
						{
							PyErr_Clear();
						}
					}
					else if (isUnit)
					{
						return true;
					}
				}
			}
		}

		return false;
	}
} // namespace Plugins
#endif
