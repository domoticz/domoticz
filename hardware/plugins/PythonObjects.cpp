#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "../../main/Logger.h"
#include "../../main/SQLHelper.h"
#include "../../hardware/hardwaretypes.h"
#include "../../main/mainstructs.h"
#include "../../main/mainworker.h"
#include "../../main/EventSystem.h"
#include "../../notifications/NotificationHelper.h"
#include "Plugins.h"
#include "PythonObjects.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"
#include <datetime.h>

namespace Plugins {

	PyTypeObject* CDeviceType = nullptr;
	PyTypeObject* CConnectionType = nullptr;
	PyTypeObject* CImageType = nullptr;

	extern struct PyModuleDef DomoticzModuleDef;
	extern struct PyModuleDef DomoticzExModuleDef;

	void CImage_dealloc(CImage* self)
	{
		Py_XDECREF(self->Base);
		Py_XDECREF(self->Name);
		Py_XDECREF(self->Description);

		PyNewRef	pType = PyObject_Type((PyObject*)self);
		freefunc pFree = (freefunc)PyType_GetSlot(pType, Py_tp_free);
		pFree((PyObject*)self);
	}

	PyObject* CImage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		allocfunc pAlloc = (allocfunc)PyType_GetSlot(type, Py_tp_alloc);
		CImage *self = (CImage *)pAlloc(type, 0);

		try
		{
			if (self == nullptr)
			{
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->ImageID = -1;
				self->Base = PyUnicode_FromString("");
				if (self->Base == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Name = PyUnicode_FromString("");
				if (self->Name == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Description = PyUnicode_FromString("");
				if (self->Description == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Filename = PyUnicode_FromString("");
				if (self->Filename == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->pPlugin = nullptr;
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

	int CImage_init(CImage *self, PyObject *args, PyObject *kwds)
	{
		char *szFileName = nullptr;
		static char *kwlist[] = { "Filename", nullptr };

		try
		{
			PyBorrowedRef pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				pModule = PyState_FindModule(&DomoticzExModuleDef);
				if (!pModule)
				{
					_log.Log(LOG_ERROR, "(%s) Domoticz/DomoticzEx modules not found in interpreter.", __func__);
					return 0;
				}
			}

			module_state*	pModState = ((struct module_state*)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "CImage:%s, unable to obtain module state.", __func__);
				return 0;
			}

			if (!pModState->pPlugin)
			{
				_log.Log(LOG_ERROR, "CImage:%s, illegal operation, Plugin has not started yet.", __func__);
				return 0;
			}

			if (PyArg_ParseTupleAndKeywords(args, kwds, "|s", kwlist, &szFileName))
			{
				std::string	sFileName = szFileName ? szFileName : "";

				if (sFileName.length())
				{
					self->pPlugin = pModState->pPlugin;
					sFileName = self->pPlugin->m_HomeFolder + sFileName;
					Py_DECREF(self->Filename);
					self->Filename = PyUnicode_FromString(sFileName.c_str());
				}
			}
			else
			{
				pModState->pPlugin->Log(LOG_ERROR, "Expected: myVar = Domoticz.Image(Filename=\"MyImages.zip\")");
				pModState->pPlugin->LogPythonException(__func__);
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

	PyObject* CImage_insert(CImage* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			std::string	sFilename = PyUnicode_AsUTF8(self->Filename);
			if (self->ImageID == -1)
			{
				if (sFilename.length())
				{
					if (self->pPlugin->m_bDebug & PDM_IMAGE)
					{
						_log.Log(LOG_NORM, "(%s) Creating images from file '%s'.", self->pPlugin->m_Name.c_str(), sFilename.c_str());
					}

					//
					//	Call code to do insert here
					//
					std::string ErrorMessage;
					if (!m_sql.InsertCustomIconFromZipFile(sFilename, ErrorMessage))
					{
						_log.Log(LOG_ERROR, "(%s) Insert Custom Icon From Zip failed on file '%s' with error '%s'.", self->pPlugin->m_Name.c_str(), sFilename.c_str(), ErrorMessage.c_str());
					}
					else
					{
						// load associated custom images to make them available to python
						std::vector<std::vector<std::string> > result = m_sql.safe_query("SELECT max(ID), Base, Name, Description FROM CustomImages");
						if (!result.empty())
						{
							// Add image objects into the image dictionary with ID as the key
							for (const auto &sd : result)
							{
								CImage *pImage = (CImage *)CImage_new(CImageType, (PyObject *)nullptr,
												      (PyObject *)nullptr);

								PyObject*	pKey = PyUnicode_FromString(sd[1].c_str());
								if (PyDict_SetItem((PyObject*)self->pPlugin->m_ImageDict, pKey, (PyObject*)pImage) == -1)
								{
									_log.Log(LOG_ERROR, "(%s) failed to add ID '%s' to image dictionary.", self->pPlugin->m_PluginKey.c_str(), sd[0].c_str());
									break;
								}
								pImage->ImageID = atoi(sd[0].c_str()) + 100;
								pImage->Base = PyUnicode_FromString(sd[1].c_str());
								pImage->Name = PyUnicode_FromString(sd[2].c_str());
								pImage->Description = PyUnicode_FromString(sd[3].c_str());
								Py_DECREF(pImage);
							}
						}
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) No images loaded.", self->pPlugin->m_Name.c_str());
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Image creation failed, '%s' already exists in Domoticz with Image ID '%d'.", self->pPlugin->m_Name.c_str(), sName.c_str(), self->ImageID);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Image creation failed, Image object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CImage_delete(CImage* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			if (self->ImageID != -1)
			{
				if (self->pPlugin->m_bDebug & PDM_IMAGE)
				{
					_log.Log(LOG_NORM, "Deleting Image '%s'.", sName.c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM CustomImages WHERE (ID==%d)", self->ImageID);
				if (!result.empty())
				{
					m_sql.safe_query("DELETE FROM CustomImages WHERE (ID==%d)", self->ImageID);

					PyNewRef	pKey = PyLong_FromLong(self->ImageID);
					if (PyDict_DelItem((PyObject*)self->pPlugin->m_ImageDict, pKey) == -1)
					{
						self->pPlugin->Log(LOG_ERROR, "Failed to delete image '%d' from images dictionary.", self->ImageID);
						Py_RETURN_NONE;
					}
				}
				else
				{
					self->pPlugin->Log(LOG_ERROR, "Image deletion failed, Image %d not found in Domoticz.", self->ImageID);
				}
			}
			else
			{
				self->pPlugin->Log(LOG_ERROR, "Image deletion failed, '%s' does not represent a Image in Domoticz.", sName.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Image deletion failed, Image object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CImage_str(CImage* self)
	{
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Base: '%U', Name: %U, Description: '%U'", self->ImageID, self->Base, self->Name, self->Description);
		return pRetVal;
	}

	void CDevice_dealloc(CDevice* self)
	{
		Py_XDECREF(self->PluginKey);
		Py_XDECREF(self->DeviceID);
		Py_XDECREF(self->Name);
		Py_XDECREF(self->LastUpdate);
		Py_XDECREF(self->Description);
		Py_XDECREF(self->sValue);
		PyDict_Clear(self->Options);
		Py_XDECREF(self->Options);
		Py_XDECREF(self->Color);

		PyNewRef	pType = PyObject_Type((PyObject*)self);
		freefunc pFree = (freefunc)PyType_GetSlot(pType, Py_tp_free);
		pFree((PyObject*)self);
	}

	PyObject* CDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		allocfunc pAlloc = (allocfunc)PyType_GetSlot(type, Py_tp_alloc);
		CDevice *self = (CDevice*)pAlloc(type, 0);

		try
		{
			if (self == nullptr)
			{
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->PluginKey = PyUnicode_FromString("");
				if (self->PluginKey == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->HwdID = -1;
				self->DeviceID = PyUnicode_FromString("");
				if (self->DeviceID == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Unit = -1;
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
				self->Description = PyUnicode_FromString("");
				if (self->Description == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->nValue = 0;
				self->sValue = PyUnicode_FromString("");
				if (self->sValue == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Options = PyDict_New();
				if (self->Options == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Image = 0;
				self->Used = 0;
				self->SignalLevel = 100;
				self->BatteryLevel = 255;
				self->TimedOut = 0;
				self->Color = PyUnicode_FromString(NoColor.toJSONString().c_str());
				if (self->Color == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->pPlugin = nullptr;
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

	extern void maptypename(const std::string &sTypeName, int &Type, int &SubType, int &SwitchType, std::string &sValue, PyObject* OptionsIn, PyObject* OptionsOut)
	{
		Type = pTypeGeneral;

		if (sTypeName == "Pressure")					SubType = sTypePressure;
		else if (sTypeName == "Percentage")				SubType = sTypePercentage;
		else if (sTypeName == "Gas")
		{
			Type = pTypeP1Gas;
			SubType = sTypeP1Gas;
		}
		else if (sTypeName == "Voltage")				SubType = sTypeVoltage;
		else if (sTypeName == "Text")					SubType = sTypeTextStatus;
		else if (sTypeName == "Switch")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
		}
		else if (sTypeName == "Alert")
		{
			sValue = "No Alert!";
			SubType = sTypeAlert;
		}
		else if (sTypeName == "Current/Ampere")
		{
			sValue = "0.0;0.0;0.0";
			Type = pTypeCURRENT;
			SubType = sTypeELEC1;
		}
		else if (sTypeName == "Sound Level")			SubType = sTypeSoundLevel;
		else if (sTypeName == "Barometer")
		{
			sValue = "1021.34;0";
			SubType = sTypeBaro;
		}
		else if (sTypeName == "Visibility")				SubType = sTypeVisibility;
		else if (sTypeName == "Distance")				SubType = sTypeDistance;
		else if (sTypeName == "Counter Incremental")	SubType = sTypeCounterIncremental;
		else if (sTypeName == "Soil Moisture")			SubType = sTypeSoilMoisture;
		else if (sTypeName == "Leaf Wetness")			SubType = sTypeLeafWetness;
		else if (sTypeName == "kWh")
		{
			sValue = "0; 0.0";
			SubType = sTypeKwh;
		}
		else if (sTypeName == "Current (Single)")		SubType = sTypeCurrent;
		else if (sTypeName == "Solar Radiation")		SubType = sTypeSolarRadiation;
		else if (sTypeName == "Temperature")
		{
			Type = pTypeTEMP;
			SubType = sTypeTEMP5;
		}
		else if (sTypeName == "Humidity")
		{
			Type = pTypeHUM;
			SubType = sTypeHUM1;
		}
		else if (sTypeName == "Temp+Hum")
		{
			sValue = "0.0;50;1";
			Type = pTypeTEMP_HUM;
			SubType = sTypeTH1;
		}
		else if (sTypeName == "Temp+Hum+Baro")
		{
			sValue = "0.0;50;1;1010;1";
			Type = pTypeTEMP_HUM_BARO;
			SubType = sTypeTHB1;
		}
		else if (sTypeName == "Wind")
		{
			sValue = "0;N;0;0;0;0";
			Type = pTypeWIND;
			SubType = sTypeWIND1;
		}
		else if (sTypeName == "Rain")
		{
			sValue = "0;0";
			Type = pTypeRAIN;
			SubType = sTypeRAIN3;
		}
		else if (sTypeName == "UV")
		{
			sValue = "0;0";
			Type = pTypeUV;
			SubType = sTypeUV1;
		}
		else if (sTypeName == "Air Quality")
		{
			Type = pTypeAirQuality;
			SubType = sTypeVoc;
		}
		else if (sTypeName == "Usage")
		{
			Type = pTypeUsage;
			SubType = sTypeElectric;
		}
		else if (sTypeName == "Illumination")
		{
			Type = pTypeLux;
			SubType = sTypeLux;
		}
		else if (sTypeName == "Waterflow")				SubType = sTypeWaterflow;
		else if (sTypeName == "Wind+Temp+Chill")
		{
			sValue = "0;N;0;0;0;0";
			Type = pTypeWIND;
			SubType = sTypeWIND4;
		}
		else if (sTypeName == "Selector Switch")
		{
			if (!OptionsIn || !PyBorrowedRef(OptionsIn).IsDict()) {
				PyDict_Clear(OptionsOut);
				PyDict_SetItemString(OptionsOut, "LevelActions", PyUnicode_FromString("|||"));
				PyDict_SetItemString(OptionsOut, "LevelNames", PyUnicode_FromString("Off|Level1|Level2|Level3"));
				PyDict_SetItemString(OptionsOut, "LevelOffHidden", PyUnicode_FromString("false"));
				PyDict_SetItemString(OptionsOut, "SelectorStyle", PyUnicode_FromString("0"));
			}
			Type = pTypeGeneralSwitch;
			SubType = sSwitchTypeSelector;
			SwitchType = STYPE_Selector;
		}
		else if (sTypeName == "Push On")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
			SwitchType = STYPE_PushOn;
		}
		else if (sTypeName == "Push Off")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
			SwitchType = STYPE_PushOff;
		}
		else if (sTypeName == "Contact")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
			SwitchType = STYPE_Contact;
		}
		else if (sTypeName == "Dimmer")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
			SwitchType = STYPE_Dimmer;
		}
		else if (sTypeName == "Motion")
		{
			Type = pTypeGeneralSwitch;
			SubType = sSwitchGeneralSwitch;
			SwitchType = STYPE_Motion;
		}
		else if (sTypeName == "Custom")
		{
			SubType = sTypeCustom;
			if (!OptionsIn || !PyBorrowedRef(OptionsIn).IsDict()) {
				PyDict_Clear(OptionsOut);
				PyDict_SetItemString(OptionsOut, "Custom", PyUnicode_FromString("1"));
			}
		}
		else if (sTypeName == "Security Panel")
		{
			Type = pTypeSecurity1;
			SubType = sTypeDomoticzSecurity;
		}
		else if (sTypeName == "Set Point" || sTypeName == "Setpoint" || sTypeName == "Thermostat")
		{
			Type = pTypeSetpoint;
			SubType = sTypeSetpoint;
		}
	}

	int CDevice_init(CDevice *self, PyObject *args, PyObject *kwds)
	{
		char *Name = nullptr;
		char *DeviceID = nullptr;
		int			Unit = -1;
		char *TypeName = nullptr;
		int			Type = -1;
		int			SubType = -1;
		int			SwitchType = -1;
		int			Image = -1;
		PyObject *Options = nullptr;
		int			Used = -1;
		char *Description = nullptr;
		static char *kwlist[] = { "Name",  "Unit",    "TypeName", "Type",     "Subtype",     "Switchtype",
					  "Image", "Options", "Used",	  "DeviceID", "Description", nullptr };

		try
		{
			PyBorrowedRef pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "(%s) Domoticz module not found in interpreter.", __func__);
				return 0;
			}

			module_state*	pModState = ((struct module_state*)PyModule_GetState(pModule));
			if (!pModState)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, unable to obtain module state.", __func__);
				return 0;
			}

			if (!pModState->pPlugin)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, illegal operation, Plugin has not started yet.", __func__);
				return 0;
			}

			if (PyArg_ParseTupleAndKeywords(args, kwds, "si|siiiiOiss", kwlist, &Name, &Unit, &TypeName, &Type, &SubType, &SwitchType, &Image, &Options, &Used, &DeviceID, &Description))
			{
				self->pPlugin = pModState->pPlugin;
				self->PluginKey = PyUnicode_FromString(pModState->pPlugin->m_PluginKey.c_str());
				self->HwdID = pModState->pPlugin->m_HwdID;
				if (Name) {
					Py_DECREF(self->Name);
					self->Name = PyUnicode_FromString(Name);
				}
				if (Description) {
					Py_DECREF(self->Description);
					self->Description = PyUnicode_FromString(Description);
				}
				if ((Unit > 0) && (Unit < 256))
				{
					self->Unit = Unit;
				}
				else
				{
					_log.Log(LOG_ERROR, "CPlugin:%s, illegal Unit number (%d), valid values range from 1 to 255.", __func__, Unit);
					return 0;
				}
				Py_DECREF(self->DeviceID);
				if (DeviceID) {
					self->DeviceID = PyUnicode_FromString(DeviceID);
				}
				else
				{
					char szID[40];		// Generate a Device ID if one was not supplied
					sprintf(szID, "%04X%04X", self->HwdID, self->Unit);
					self->DeviceID = PyUnicode_FromString(szID);
				}
				if (TypeName) {
					std::string sValue;
					maptypename(std::string(TypeName), self->Type, self->SubType, self->SwitchType, sValue, Options, self->Options);
					Py_DECREF(self->sValue);
					self->sValue = PyUnicode_FromString(sValue.c_str());
				}
				if ((Type != -1) && Type) self->Type = Type;
				if ((SubType != -1) && SubType) self->SubType = SubType;
				if (SwitchType != -1) self->SwitchType = SwitchType;
				if (Image != -1) self->Image = Image;
				if (Used == 1) self->Used = Used;
				if (Options && PyBorrowedRef(Options).IsDict() && PyDict_Size(Options) > 0) {
					PyObject *pKey, *pValue;
					Py_ssize_t pos = 0;
					PyDict_Clear(self->Options);
					while (PyDict_Next(Options, &pos, &pKey, &pValue))
					{
						PyNewRef	pKeyDict = PyObject_Str(pKey);
						PyNewRef	pValueDict = PyObject_Str(pValue);

						if (pKeyDict && pValueDict)
						{
							if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
							{
								pModState->pPlugin->Log(LOG_ERROR, "(%s) Failed to initialize Options dictionary for Hardware/Unit combination (%d:%d).",
									pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit);
								break;
							}
						}
						else
						{
							PyNewRef	pName = PyObject_GetAttrString((PyObject*)pValue->ob_type, "__name__");
							pModState->pPlugin->Log(
								LOG_ERROR,
								"(%s) Failed to initialize Options dictionary for Hardware / Unit combination(%d:%d): Unable to convert to string.)",
								pModState->pPlugin->m_Name.c_str(), pModState->pPlugin->m_HwdID, self->Unit);
						}
					}
				}
			}
			else
			{
				pModState->pPlugin->Log(LOG_ERROR, R"(Expected: myVar = Domoticz.Device(Name="myDevice", Unit=0, TypeName="", Type=0, Subtype=0, Switchtype=0, Image=0, Options={}, Used=1))");
				pModState->pPlugin->LogPythonException(__func__);
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

	PyObject* CDevice_refresh(CDevice* self)
	{
		if ((self->pPlugin) && (self->HwdID != -1) && (self->Unit != -1))
		{
			// load associated devices to make them available to python
			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, DeviceID, Type, SubType, SwitchType, LastLevel, CustomImage, SignalLevel, BatteryLevel, LastUpdate, Options, Description, Color, Used FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) ORDER BY Unit ASC", self->HwdID, self->Unit);
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
					Py_XDECREF(self->DeviceID);
					self->DeviceID = PyUnicode_FromString(sd[5].c_str());
					self->Type = atoi(sd[6].c_str());
					self->SubType = atoi(sd[7].c_str());
					self->SwitchType = atoi(sd[8].c_str());
					self->LastLevel = atoi(sd[9].c_str());
					self->Image = atoi(sd[10].c_str());
					self->SignalLevel = atoi(sd[11].c_str());
					self->BatteryLevel = atoi(sd[12].c_str());
					Py_XDECREF(self->LastUpdate);
					self->LastUpdate = PyUnicode_FromString(sd[13].c_str());
					PyDict_Clear(self->Options);
					if (!sd[14].empty())
					{
						if (self->SubType == sTypeCustom)
						{
							PyDict_SetItemString(self->Options, "Custom", PyUnicode_FromString(sd[14].c_str()));
						}
						else
						{
							std::map<std::string, std::string> mpOptions;
							Py_BEGIN_ALLOW_THREADS
							mpOptions =	m_sql.BuildDeviceOptions(sd[14], true);
							Py_END_ALLOW_THREADS
							for (const auto &opt : mpOptions)
							{
								PyNewRef	pKeyDict = PyUnicode_FromString(opt.first.c_str());
								PyNewRef	pValueDict = PyUnicode_FromString(opt.second.c_str());
								if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
								{
									_log.Log(LOG_ERROR, "(%s) Failed to refresh Options dictionary for Hardware/Unit combination (%d:%d).", self->pPlugin->m_Name.c_str(), self->HwdID, self->Unit);
									break;
								}
							}
						}
					}
					Py_XDECREF(self->Description);
					self->Description = PyUnicode_FromString(sd[15].c_str());
					Py_XDECREF(self->Color);
					self->Color = PyUnicode_FromString(_tColor(std::string(sd[16])).toJSONString().c_str()); //Parse the color to detect incorrectly formatted color data
					self->Used = atoi(sd[17].c_str());
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device refresh failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CDevice_insert(CDevice* self)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			std::string	sDeviceID = PyUnicode_AsUTF8(self->DeviceID);
			if (self->ID == -1)
			{
				if (self->pPlugin->m_bDebug & PDM_DEVICE)
				{
					self->pPlugin->Log(LOG_NORM, "Creating device '%s'.", sName.c_str());
				}

				if (!m_sql.m_bAcceptNewHardware)
				{
					self->pPlugin->Log(LOG_ERROR, "Device creation failed, Domoticz settings prevent accepting new devices.");
				}
				else
				{
					std::vector<std::vector<std::string> > result;
					result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
					if (result.empty())
					{
						std::string	sValue = PyUnicode_AsUTF8(self->sValue);
						std::string	sColor = _tColor(std::string(PyUnicode_AsUTF8(self->Color))).toJSONString(); //Parse the color to detect incorrectly formatted color data
						std::string	sLongName = self->pPlugin->m_Name + " - " + sName;
						std::string	sDescription = PyUnicode_AsUTF8(self->Description);
						if ((self->SubType == sTypeCustom) && (PyDict_Size(self->Options) > 0))
						{
							PyBorrowedRef	pValueDict = PyDict_GetItemString(self->Options, "Custom");
							std::string sOptionValue;
							if (!pValueDict)
								sOptionValue = "";
							else
								sOptionValue = PyUnicode_AsUTF8(pValueDict);

							m_sql.safe_query(
								"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Description, Color, Options) "
								"VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q', '%q', '%q')",
								self->HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used, sLongName.c_str(), sValue.c_str(), self->Image, sDescription.c_str(), sColor.c_str(), sOptionValue.c_str());
						}
						else
						{
							m_sql.safe_query(
								"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Description, Color) "
								"VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q', '%q')",
								self->HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used, sLongName.c_str(), sValue.c_str(), self->Image, sDescription.c_str(), sColor.c_str());
						}

						result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
						if (!result.empty())
						{
							self->ID = atoi(result[0][0].c_str());

							PyNewRef	pKey = PyLong_FromLong(self->Unit);
							if (PyDict_SetItem((PyObject*)self->pPlugin->m_DeviceDict, pKey, (PyObject*)self) == -1)
							{
								self->pPlugin->Log(LOG_ERROR, "Failed to add unit '%d' to device dictionary.", self->Unit);
								Py_RETURN_NONE;
							}

							// Device successfully created, now set the options when supplied
							if ((self->SubType != sTypeCustom) && (PyDict_Size(self->Options) > 0))
							{
								PyBorrowedRef	pKeyDict, pValueDict;
								Py_ssize_t pos = 0;
								std::map<std::string, std::string> mpOptions;
								while (PyDict_Next(self->Options, &pos, &pKeyDict, &pValueDict)) {
									std::string sOptionName = pKeyDict;
									std::string sOptionValue = pValueDict;
									mpOptions.insert(std::pair<std::string, std::string>(sOptionName, sOptionValue));
								}
								m_sql.SetDeviceOptions(self->ID, mpOptions);
							}

							// Refresh device data to ensure it is usable straight away
							PyObject* pRetVal = CDevice_refresh(self);
							Py_DECREF(pRetVal);
						}
						else
						{
							self->pPlugin->Log(LOG_ERROR, "Device creation failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->HwdID, self->Unit);
						}
					}
					else
					{
						self->pPlugin->Log(LOG_ERROR, "Device creation failed, Hardware/Unit combination (%d:%d) already exists in Domoticz.", self->HwdID, self->Unit);
					}
				}
			}
			else
			{
				self->pPlugin->Log(LOG_ERROR, "Device creation failed, '%s' already exists in Domoticz with Device ID '%d'.", sName.c_str(), self->ID);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device creation failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CDevice_update(CDevice *self, PyObject *args, PyObject *kwds)
	{
		if (self->pPlugin)
		{
			self->pPlugin->SetHeartbeatReceived();

			int			nValue = self->nValue;
			char *sValue = nullptr;
			int			iSignalLevel = self->SignalLevel;
			int			iBatteryLevel = self->BatteryLevel;
			int			iImage = self->Image;
			int			iTimedOut = self->TimedOut;
			PyObject *pOptionsDict = nullptr;

			char *Name = nullptr;
			char *TypeName = nullptr;
			int			iType = self->Type;
			int			iSubType = self->SubType;
			int			iSwitchType = self->SwitchType;
			int			iUsed = self->Used;
			uint64_t 	DevRowIdx;
			char *Description = nullptr;
			char *Color = nullptr;
			int			SuppressTriggers = false;

			std::string	sName = PyUnicode_AsUTF8(self->Name);
			std::string	sDeviceID = PyUnicode_AsUTF8(self->DeviceID);
			std::string	sDescription = PyUnicode_AsUTF8(self->Description);
			static char *kwlist[]
				= { "nValue", "sValue",		  "Image", "SignalLevel", "BatteryLevel", "Options", "TimedOut",
				    "Name",   "TypeName",	  "Type",  "Subtype",	  "Switchtype",	  "Used",    "Description",
				    "Color",  "SuppressTriggers", nullptr };

			// Try to extract parameters needed to update device settings
			if (!PyArg_ParseTupleAndKeywords(args, kwds,   "is|iiiOissiiiissp", kwlist, &nValue, &sValue, &iImage, &iSignalLevel, &iBatteryLevel, &pOptionsDict, &iTimedOut, &Name, &TypeName, &iType, &iSubType, &iSwitchType, &iUsed, &Description, &Color, &SuppressTriggers))
			{
				self->pPlugin->Log(LOG_ERROR, "(%s) %s: Failed to parse parameters: 'nValue', 'sValue', 'Image', 'SignalLevel', 'BatteryLevel', 'Options', 'TimedOut', 'Name', 'TypeName', 'Type', 'Subtype', 'Switchtype', 'Used', 'Description', 'Color' or 'SuppressTriggers' expected.", __func__, sName.c_str());
				self->pPlugin->LogPythonException(__func__);
				Py_RETURN_NONE;
			}

			std::string sID = std::to_string(self->ID);

			// Name change
			if (Name)
			{
				sName = Name;
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("Name", sName, sID);
				Py_END_ALLOW_THREADS
			}

			// Description change
			if (Description)
			{
				std::string sDescription = Description;
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("Description", sDescription, sID);
				Py_END_ALLOW_THREADS
			}

			// TypeName change - actually derives new Type, SubType and SwitchType values
			if (TypeName) {
				std::string stdsValue;
				maptypename(std::string(TypeName), iType, iSubType, iSwitchType, stdsValue, pOptionsDict, pOptionsDict);

				// Reset nValue and sValue when changing device types
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("nValue", 0, sID);
				m_sql.UpdateDeviceValue("sValue", stdsValue, sID);
				Py_END_ALLOW_THREADS
			}

			// Type change
			if (iType != self->Type)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("Type", iType, sID);
				Py_END_ALLOW_THREADS
			}

			// SubType change
			if (iSubType != self->SubType)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("SubType", iSubType, sID);
				Py_END_ALLOW_THREADS
			}

			// SwitchType change
			if (iSwitchType != self->SwitchType)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("SwitchType", iSwitchType, sID);
				Py_END_ALLOW_THREADS
			}

			// Image change
			if (iImage != self->Image)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("CustomImage", iImage, sID);
				Py_END_ALLOW_THREADS
			}

			// BatteryLevel change
			if (iBatteryLevel != self->BatteryLevel)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("BatteryLevel", iBatteryLevel, sID);
				Py_END_ALLOW_THREADS
			}

			// SignalLevel change
			if (iSignalLevel != self->SignalLevel)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("SignalLevel", iSignalLevel, sID);
				Py_END_ALLOW_THREADS
			}

			// Used change
			if (iUsed != self->Used)
			{
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("Used", iUsed, sID);
				Py_END_ALLOW_THREADS
			}

			// Color change
			if (Color)
			{
				std::string	sColor = _tColor(std::string(Color)).toJSONString(); //Parse the color to detect incorrectly formatted color data
				Py_BEGIN_ALLOW_THREADS
				m_sql.UpdateDeviceValue("Color", sColor, sID);
				Py_END_ALLOW_THREADS
			}

			// Options provided, assume change
			if (pOptionsDict && PyBorrowedRef(pOptionsDict).IsDict())
			{
				if (self->SubType != sTypeCustom)
				{
					PyBorrowedRef	pKeyDict, pValueDict;
					Py_ssize_t pos = 0;
					std::map<std::string, std::string> mpOptions;
					while (PyDict_Next(pOptionsDict, &pos, &pKeyDict, &pValueDict))
					{
						std::string sOptionName = pKeyDict;
						std::string sOptionValue = pValueDict;
						mpOptions.insert(std::pair<std::string, std::string>(sOptionName, sOptionValue));
					}
					Py_BEGIN_ALLOW_THREADS
					m_sql.SetDeviceOptions(self->ID, mpOptions);
					Py_END_ALLOW_THREADS
				}
				else
				{
					std::string sOptionValue;
					PyBorrowedRef	pValue = PyDict_GetItemString(pOptionsDict, "Custom");
					if (pValue)
					{
						sOptionValue = PyUnicode_AsUTF8(pValue);
					}

					std::string sLastUpdate = TimeToString(nullptr, TF_DateTime);
					Py_BEGIN_ALLOW_THREADS
					m_sql.UpdateDeviceValue("Options", iUsed, sID);
					m_sql.safe_query("UPDATE DeviceStatus SET Options='%q', LastUpdate='%q' WHERE (HardwareID==%d) and (Unit==%d)",
						sOptionValue.c_str(), sLastUpdate.c_str(), self->HwdID, self->Unit);
					Py_END_ALLOW_THREADS
				}
			}

			// TimedOut change (not stored in database, webserver calls back directly to check)
			if (iTimedOut != self->TimedOut)
			{
				self->TimedOut = iTimedOut;
			}

			// Suppress Triggers updates non-key fields only (specifically NOT nValue or sValue)
			if (!SuppressTriggers)
			{
				if (self->pPlugin->m_bDebug & PDM_DEVICE)
				{
					_log.Log(LOG_NORM, "(%s) Updating device from %d:'%s' to have values %d:'%s'.", sName.c_str(), self->nValue, PyUnicode_AsUTF8(self->sValue), nValue, sValue);
				}
				Py_BEGIN_ALLOW_THREADS
				DevRowIdx = m_sql.UpdateValue(self->HwdID, sDeviceID.c_str(), (const unsigned char)self->Unit, (const unsigned char)iType, (const unsigned char)iSubType, iSignalLevel, iBatteryLevel, nValue, sValue, sName, true);
				Py_END_ALLOW_THREADS
				// if this is an internal Security Panel then there are some extra updates required if state has changed
				if ((self->Type == pTypeSecurity1) && (self->SubType == sTypeDomoticzSecurity) && (self->nValue != nValue))
				{
					switch (nValue)
					{
					case sStatusArmHome:
					case sStatusArmHomeDelayed:
						Py_BEGIN_ALLOW_THREADS
						m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDHOME);
						m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDHOME, "Python");
						Py_END_ALLOW_THREADS
						break;
					case sStatusArmAway:
					case sStatusArmAwayDelayed:
						Py_BEGIN_ALLOW_THREADS
						m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_ARMEDAWAY);
						m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_ARMEDAWAY, "Python");
						Py_END_ALLOW_THREADS
						break;
					case sStatusDisarm:
					case sStatusNormal:
					case sStatusNormalDelayed:
					case sStatusNormalTamper:
					case sStatusNormalDelayedTamper:
						Py_BEGIN_ALLOW_THREADS
						m_sql.UpdatePreferencesVar("SecStatus", SECSTATUS_DISARMED);
						m_mainworker.UpdateDomoticzSecurityStatus(SECSTATUS_DISARMED, "Python");
						Py_END_ALLOW_THREADS
						break;
					}
				}

				// Notify MQTT and various push mechanisms and notifications
				Py_BEGIN_ALLOW_THREADS
				m_mainworker.sOnDeviceReceived(self->pPlugin->m_HwdID, self->ID, self->pPlugin->m_Name, NULL);
				m_notifications.CheckAndHandleNotification(DevRowIdx, self->HwdID, sDeviceID, sName, self->Unit, iType, iSubType, nValue, sValue);

				// Trigger any associated scene / groups
				m_mainworker.CheckSceneCode(DevRowIdx, (const unsigned char)self->Type, (const unsigned char)self->SubType, nValue, sValue, "Python");
				Py_END_ALLOW_THREADS

			}

			CDevice_refresh(self);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device update failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CDevice_delete(CDevice* self)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			if (self->ID != -1)
			{
				if (self->pPlugin->m_bDebug & PDM_DEVICE)
				{
					self->pPlugin->Log(LOG_NORM, "Deleting device '%s'.", sName.c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
				if (!result.empty())
				{
					m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);

					PyNewRef	pKey = PyLong_FromLong(self->Unit);
					if (PyDict_DelItem((PyObject*)self->pPlugin->m_DeviceDict, pKey) == -1)
					{
						self->pPlugin->Log(LOG_ERROR, "Failed to delete unit '%d' from device dictionary.", self->Unit);
						Py_RETURN_NONE;
					}
				}
				else
				{
					self->pPlugin->Log(LOG_ERROR, "Device deletion failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->HwdID, self->Unit);
				}
			}
			else
			{
				self->pPlugin->Log(LOG_ERROR, "Device deletion failed, '%s' does not represent a device in Domoticz.", sName.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device deletion failed, Device object is not associated with a plugin.");
		}

		Py_RETURN_NONE;
	}

	PyObject* CDevice_touch(CDevice * self)
	{
		Py_BEGIN_ALLOW_THREADS
		if ((self->pPlugin) && (self->HwdID != -1) && (self->Unit != -1))
		{
			self->pPlugin->SetHeartbeatReceived();
			std::string sID = std::to_string(self->ID);
			m_sql.safe_query("UPDATE DeviceStatus SET LastUpdate='%q' WHERE (ID == %s )",
					 TimeToString(nullptr, TF_DateTime).c_str(), sID.c_str());
		}
		else
		{
			_log.Log(LOG_ERROR, "Device touch failed, Device object is not associated with a plugin.");
		}
		Py_END_ALLOW_THREADS
		return CDevice_refresh(self);
	}

	PyObject* CDevice_str(CDevice* self)
	{
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Name: '%U', nValue: %d, sValue: '%U'", self->ID, self->Name, self->nValue, self->sValue);
		return pRetVal;
	}

	void CConnection_dealloc(CConnection * self)
	{
		CPlugin *pPlugin = self->pPlugin;
		if (!pPlugin)
		{
			pPlugin = CPlugin::FindPlugin();
		}
		if (pPlugin && (pPlugin->m_bDebug & PDM_CONNECTION))
		{
			pPlugin->Log(LOG_NORM, "Deallocating connection object '%s' (%s:%s).", PyUnicode_AsUTF8(self->Name), PyUnicode_AsUTF8(self->Address), PyUnicode_AsUTF8(self->Port));
		}

		Py_XDECREF(self->Target);
		Py_XDECREF(self->Address);
		Py_XDECREF(self->Port);
		Py_XDECREF(self->LastSeen);
		Py_XDECREF(self->Transport);
		Py_XDECREF(self->Protocol);
		Py_XDECREF(self->Parent);

		if (self->pTransport)
		{
			delete self->pTransport;
			self->pTransport = nullptr;
		}
		if (self->pProtocol)
		{
			delete self->pProtocol;
			self->pProtocol = nullptr;
		}

		PyNewRef	pType = PyObject_Type((PyObject*)self);
		freefunc pFree = (freefunc)PyType_GetSlot(pType, Py_tp_free);
		pFree((PyObject*)self);
	}

	PyObject * CConnection_new(PyTypeObject * type, PyObject * args, PyObject * kwds)
	{
		allocfunc pAlloc = (allocfunc)PyType_GetSlot(type, Py_tp_alloc);
		CConnection *self = (CConnection*)pAlloc(type, 0);

		try
		{
			if (self == nullptr)
			{
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->Name = PyUnicode_FromString("");
				if (self->Name == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Target = NULL;
				self->Address = PyUnicode_FromString("");
				if (self->Address == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Port = PyUnicode_FromString("");
				if (self->Port == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->LastSeen = PyUnicode_FromString("");
				if (self->LastSeen == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Transport = PyUnicode_FromString("");
				if (self->Transport == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}
				self->Protocol = PyUnicode_FromString("None");
				if (self->Protocol == nullptr)
				{
					Py_DECREF(self);
					return nullptr;
				}

				self->Parent = Py_None;
				Py_INCREF(Py_None);

				self->pTransport = nullptr;
				self->pProtocol = nullptr;
				self->pPlugin = nullptr;
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

	int CConnection_init(CConnection * self, PyObject * args, PyObject * kwds)
	{
		char *pName = nullptr;
		char *pTransport = nullptr;
		char *pProtocol = nullptr;
		char *pAddress = nullptr;
		char *pPort = nullptr;
		int			iBaud = -1;
		static char *kwlist[] = { "Name", "Transport", "Protocol", "Address", "Port", "Baud", nullptr };

		try
		{
			if (PyArg_ParseTupleAndKeywords(args, kwds, "ss|sssi", kwlist, &pName, &pTransport, &pProtocol, &pAddress, &pPort, &iBaud))
			{
				if (pName) {
					Py_XDECREF(self->Name);
					self->Name = PyUnicode_FromString(pName);
				}
				if (pAddress) {
					Py_XDECREF(self->Address);
					self->Address = PyUnicode_FromString(pAddress);
				}
				if (pPort) {
					Py_XDECREF(self->Port);
					self->Port = PyUnicode_FromString(pPort);
				}
				if (iBaud) self->Baud = iBaud;
				if (pTransport)
				{
					Py_XDECREF(self->Transport);
					self->Transport = PyUnicode_FromString(pTransport);
				}
				if (pProtocol)
				{
					Py_XDECREF(self->Protocol);
					self->Protocol = PyUnicode_FromString(pProtocol);
				}
			}
			else
			{
				_log.Log(LOG_ERROR,
					 R"(Expected: myVar = Domoticz.Connection(Name="<Name>", Transport="<Transport>", Protocol="<Protocol>", Address="<IP-Address>", Port="<Port>", Baud=0))");
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

	PyObject *CConnection_connect(CConnection *self, PyObject *args, PyObject *kwds)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!self->pPlugin)
		{
			self->pPlugin = pPlugin;
			if (!self->pPlugin)
			{
				_log.Log(LOG_ERROR, "%s:, illegal operation, Plugin has not started yet.", __func__);
				Py_RETURN_NONE;
			}
		}

		//	Add connect command to message queue unless already connected
		if (pPlugin->IsStopRequested(0))
		{
			pPlugin->Log(LOG_NORM, "%s, connect request from '%s' ignored. Plugin is stopping.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		if (self->pTransport && self->pTransport->IsConnecting())
		{
			pPlugin->Log(LOG_ERROR, "%s, connect request from '%s' ignored. Transport is connecting.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		if (self->pTransport && self->pTransport->IsConnected())
		{
			pPlugin->Log(LOG_ERROR, "%s, connect request from '%s' ignored. Transport is connected.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		PyObject *pTarget = NULL;
		int iTimeout = 0;
		static char *kwlist[] = { "Target", "Timeout", NULL };
		if (PyArg_ParseTupleAndKeywords(args, kwds, "|OI", kwlist, &pTarget, &iTimeout))
		{
			if (pTarget)
			{
				// This check is not effective enough, almost all Python variables pass it
				PyNewRef pFunc = PyObject_GetAttrString(pTarget, "__init__");
				if (!pFunc || !PyCallable_Check(pFunc))
				{
					self->pPlugin->Log(LOG_ERROR, "Object is not callable, Target parameter ignored.");
				}
				else
				{
					Py_INCREF(pTarget);
					self->Target = pTarget;
				}
			}
			if (!iTimeout || (iTimeout > 199))
			{
				self->Timeout = iTimeout;
				if (!self->pProtocol)
				{
					pPlugin->MessagePlugin(new ProtocolDirective(self));
				}
				pPlugin->MessagePlugin(new ConnectDirective(self));
			}
			else
			{
				pPlugin->Log(LOG_ERROR, "Timeout parameter ignored, must be zero or greater than 250 milliseconds.");
			}
		}

		Py_RETURN_NONE;
	}

	PyObject *CConnection_listen(CConnection *self, PyObject *args, PyObject *kwds)
	{
		CPlugin* pPlugin = CPlugin::FindPlugin();
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "%s:, illegal operation, Connection is not associated with a Plugin.", __func__);
			Py_RETURN_NONE;
		}

		//	Add connect command to message queue unless already connected
		if (pPlugin->IsStopRequested(0))
		{
			pPlugin->Log(LOG_NORM, "%s, listen request from '%s' ignored. Plugin is stopping.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		if (self->pTransport && self->pTransport->IsConnecting())
		{
			pPlugin->Log(LOG_ERROR, "%s, listen request from '%s' ignored. Transport is connecting.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		if (self->pTransport && self->pTransport->IsConnected())
		{
			pPlugin->Log(LOG_ERROR, "%s, listen request from '%s' ignored. Transport is connected.", __func__, self->pPlugin->m_Name.c_str());
			Py_RETURN_NONE;
		}

		PyObject *pTarget = NULL;
		static char *kwlist[] = { "Target", NULL };
		if (PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &pTarget))
		{
			if (pTarget)
			{
				Py_INCREF(pTarget);
				self->Target = pTarget;
			}
		}

		pPlugin->MessagePlugin(new ListenDirective(self));

		Py_RETURN_NONE;
	}

	PyObject * CConnection_send(CConnection * self, PyObject * args, PyObject * kwds)
	{
		CPlugin *pPlugin = self->pPlugin;
		if (!pPlugin)
		{
			self->pPlugin = CPlugin::FindPlugin();
			if (!self->pPlugin)
			{
				_log.Log(LOG_ERROR, "%s:, illegal operation, Connection is not associated with a Plugin.", __func__);
				Py_RETURN_NONE;
			}
			pPlugin = self->pPlugin;
		}
		
		if (pPlugin->IsStopRequested(0))
		{
			pPlugin->Log(LOG_NORM, "%s, send request from '%s' ignored. Plugin is stopping.", __func__, pPlugin->m_Name.c_str());
		}
		else
		{
			PyObject *pData = nullptr;
			int			iDelay = 0;
			static char *kwlist[] = { "Message", "Delay", nullptr };
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &pData, &iDelay))
			{
				pPlugin->Log(LOG_ERROR, "(%s) failed to parse parameters, Message or Message, Delay expected.", pPlugin->m_Name.c_str());
				pPlugin->LogPythonException(__func__);
			}
			else
			{
				//	Add start command to message queue
				pPlugin->MessagePlugin(new WriteDirective(self, pData, iDelay));
			}
		}

		Py_RETURN_NONE;
	}

	PyObject * CConnection_disconnect(CConnection * self)
	{
		CPlugin *pPlugin = self->pPlugin;
		if (!pPlugin)
		{
			_log.Log(LOG_ERROR, "%s:, illegal operation, Connection is not associated with a Plugin.", __func__);
			Py_RETURN_NONE;
		}

		if (self->pTransport)
		{
			if (self->pTransport->IsConnecting() || self->pTransport->IsConnected())
			{
				pPlugin->MessagePlugin(new DisconnectDirective(self));
			}
			else
				pPlugin->Log(LOG_ERROR, "%s, disconnection request from '%s' ignored. Transport is not connecting or connected.", __func__, pPlugin->m_Name.c_str());
		}
		else
			pPlugin->Log(LOG_ERROR, "%s, disconnection request from '%s' ignored. Transport does not exist.", __func__, pPlugin->m_Name.c_str());

		Py_RETURN_NONE;
	}

	PyObject * CConnection_bytes(CConnection * self)
	{
		if (self->pTransport)
		{
			return PyLong_FromLong(self->pTransport->TotalBytes());
		}

		return PyBool_FromLong(0);
	}

	PyObject * CConnection_isconnecting(CConnection * self)
	{
		if (self->pTransport)
		{
			return PyBool_FromLong(self->pTransport->IsConnecting());
		}

		return PyBool_FromLong(0);
	}

	PyObject * CConnection_isconnected(CConnection * self)
	{
		if (self->pTransport)
		{
			return PyBool_FromLong(self->pTransport->IsConnected());
		}

		return PyBool_FromLong(0);
	}

	PyObject * CConnection_timestamp(CConnection * self)
	{
		if (self->pTransport)
		{
			time_t	tLastSeen = self->pTransport->LastSeen();
			struct tm ltime;
			localtime_r(&tLastSeen, &ltime);
			char date[32];
			strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &ltime);
			PyObject* pLastSeen = PyUnicode_FromString(date);
			return pLastSeen;
		}

		Py_RETURN_NONE;
	}

	PyObject * CConnection_str(CConnection * self)
	{
		std::string		sParent = "None";
		if (((PyObject *)self->Parent) != Py_None)
		{
			sParent = PyUnicode_AsUTF8(((CConnection*)self->Parent)->Name);
		}

		if (self->pTransport)
		{
			time_t	tLastSeen = self->pTransport->LastSeen();
			struct tm ltime;
			localtime_r(&tLastSeen, &ltime);
			char date[32];
			strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &ltime);
			PyObject*	pRetVal = PyUnicode_FromFormat("Name: '%U', Transport: '%U', Protocol: '%U', Address: '%U', Port: '%U', Baud: %d, Timeout: %d, Bytes: %d, Connected: %s, Last Seen: %s, Parent: '%s'",
				self->Name, self->Transport, self->Protocol, self->Address, self->Port, self->Baud, self->Timeout,
				(self->pTransport ? self->pTransport->TotalBytes() : -1),
				(self->pTransport ? (self->pTransport->IsConnected() ? "True" : "False") : "False"), date, sParent.c_str());
			return pRetVal;
		}
		PyObject *pRetVal = PyUnicode_FromFormat("Name: '%U', Transport: '%U', Protocol: '%U', Address: '%U', Port: '%U', Baud: %d, Timeout: %d, Connected: False, Parent: '%s'", self->Name,
							 self->Transport, self->Protocol, self->Address, self->Port, self->Baud, self->Timeout, sParent.c_str());
		return pRetVal;
	}
} // namespace Plugins
#endif
