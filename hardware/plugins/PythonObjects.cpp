#include "stdafx.h"

//
//	Domoticz Plugin System - Dnpwwo, 2016
//
#ifdef ENABLE_PYTHON

#include "../main/Logger.h"
#include "../main/SQLHelper.h"
#include "../hardware/hardwaretypes.h"
#include "../main/localtime_r.h"
#include "../main/mainworker.h"
#include "../main/EventSystem.h"
#include "PythonObjects.h"
#include "PluginMessages.h"
#include "PluginProtocols.h"
#include "PluginTransports.h"
#include <datetime.h>

namespace Plugins {

	extern boost::mutex PluginMutex;	// controls accessto the message queue
	extern std::queue<CPluginMessageBase*>	PluginMessageQueue;
	extern boost::asio::io_service ios;
	extern struct PyModuleDef DomoticzModuleDef;
	extern void LogPythonException(CPlugin* pPlugin, const std::string &sHandler);

	struct module_state {
		CPlugin*	pPlugin;
		PyObject*	error;
	};

	void CImage_dealloc(CImage* self)
	{
		Py_XDECREF(self->Base);
		Py_XDECREF(self->Name);
		Py_XDECREF(self->Description);
		Py_TYPE(self)->tp_free((PyObject*)self);
	}

	PyObject* CImage_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		CImage *self = (CImage *)type->tp_alloc(type, 0);

		try
		{
			if (self == NULL) {
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->ImageID = -1;
				self->Base = PyUnicode_FromString("");
				if (self->Base == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Name = PyUnicode_FromString("");
				if (self->Name == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Description = PyUnicode_FromString("");
				if (self->Description == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Filename = PyUnicode_FromString("");
				if (self->Filename == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->pPlugin = NULL;
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
		char*		szFileName = NULL;
		static char *kwlist[] = { "Filename", NULL };

		try
		{
			PyObject*	pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "CImage:%s, unable to find module for current interpreter.", __func__);
				return 0;
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
				CPlugin* pPlugin = NULL;
				if (pModState) pPlugin = pModState->pPlugin;
				_log.Log(LOG_ERROR, "Expected: myVar = Domoticz.Image(Filename=\"MyImages.zip\")");
				LogPythonException(pPlugin, __func__);
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
					if (self->pPlugin->m_bDebug)
					{
						_log.Log(LOG_NORM, "(%s) Creating images from file '%s'.", self->pPlugin->Name.c_str(), sFilename.c_str());
					}

					//
					//	Call code to do insert here
					//
					std::string ErrorMessage;
					if (!m_sql.InsertCustomIconFromZipFile(sFilename, ErrorMessage))
					{
						_log.Log(LOG_ERROR, "(%s) Insert Custom Icon From Zip failed on file '%s' with error '%s'.", self->pPlugin->Name.c_str(), sFilename.c_str(), ErrorMessage.c_str());
					}
					else
					{
						// load associated custom images to make them available to python
						std::vector<std::vector<std::string> > result = m_sql.safe_query("SELECT max(ID), Base, Name, Description FROM CustomImages");
						if (result.size() > 0)
						{
							PyType_Ready(&CImageType);
							// Add image objects into the image dictionary with ID as the key
							for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
							{
								std::vector<std::string> sd = *itt;
								CImage* pImage = (CImage*)CImage_new(&CImageType, (PyObject*)NULL, (PyObject*)NULL);

								PyObject*	pKey = PyUnicode_FromString(sd[1].c_str());
								if (PyDict_SetItem((PyObject*)self->pPlugin->m_ImageDict, pKey, (PyObject*)pImage) == -1)
								{
									_log.Log(LOG_ERROR, "(%s) failed to add ID '%s' to image dictionary.", self->pPlugin->m_PluginKey.c_str(), sd[0].c_str());
									break;
								}
								else
								{
									pImage->ImageID = atoi(sd[0].c_str()) + 100;
									pImage->Base = PyUnicode_FromString(sd[1].c_str());
									pImage->Name = PyUnicode_FromString(sd[2].c_str());
									pImage->Description = PyUnicode_FromString(sd[3].c_str());
									Py_DECREF(pImage);
								}
							}
						}
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) No images loaded.", self->pPlugin->Name.c_str());
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Image creation failed, '%s' already exists in Domoticz with Image ID '%d'.", self->pPlugin->Name.c_str(), sName.c_str(), self->ImageID);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Image creation failed, Image object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CImage_delete(CImage* self, PyObject *args)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			if (self->ImageID != -1)
			{
				if (self->pPlugin->m_bDebug)
				{
					_log.Log(LOG_NORM, "(%s) Deleting Image '%s'.", self->pPlugin->Name.c_str(), sName.c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM CustomImages WHERE (ID==%d)", self->ImageID);
				if (result.size() != 0)
				{
					m_sql.safe_query("DELETE FROM CustomImages WHERE (ID==%d)", self->ImageID);

					PyObject*	pKey = PyLong_FromLong(self->ImageID);
					if (PyDict_DelItem((PyObject*)self->pPlugin->m_DeviceDict, pKey) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to delete image '%d' from images dictionary.", self->pPlugin->Name.c_str(), self->ImageID);
						Py_INCREF(Py_None);
						return Py_None;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Image deletion failed, Image %d not found in Domoticz.", self->pPlugin->Name.c_str(), self->ImageID);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Image deletion failed, '%s' does not represent a Image in Domoticz.", self->pPlugin->Name.c_str(), sName.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Image deletion failed, Image object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CImage_str(CImage* self)
	{
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Base: '%U', Name: %U, Description: '%U'", self->ImageID, self->Base, self->Name, self->Description);
		return pRetVal;
	}

	void CDevice_dealloc(CDevice* self)
	{
		Py_XDECREF(self->Name);
		Py_XDECREF(self->sValue);
		PyDict_Clear(self->Options);
		Py_XDECREF(self->Options);
		Py_TYPE(self)->tp_free((PyObject*)self);
	}

	PyObject* CDevice_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
	{
		CDevice *self = (CDevice *)type->tp_alloc(type, 0);

		try
		{
			if (self == NULL) {
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->PluginKey = PyUnicode_FromString("");
				if (self->PluginKey == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->HwdID = -1;
				self->DeviceID = PyUnicode_FromString("");
				if (self->DeviceID == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Unit = -1;
				self->Type = 0;
				self->SubType = 0;
				self->SwitchType = 0;
				self->ID = -1;
				self->LastLevel = 0;
				self->Name = PyUnicode_FromString("");
				if (self->Name == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->nValue = 0;
				self->sValue = PyUnicode_FromString("");
				if (self->sValue == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Options = PyDict_New();
				if (self->Options == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Image = 0;
				self->Used = 0;
				self->SignalLevel = 100;
				self->BatteryLevel = 255;
				self->TimedOut = 0;
				self->pPlugin = NULL;
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

	int CDevice_init(CDevice *self, PyObject *args, PyObject *kwds)
	{
		char*		Name = NULL;
		char*		DeviceID = NULL;
		int			Unit = -1;
		char*		TypeName = NULL;
		int			Type = -1;
		int			SubType = -1;
		int			SwitchType = -1;
		int			Image = -1;
		PyObject*	Options = NULL;
		int			Used = -1;
		static char *kwlist[] = { "Name", "Unit", "TypeName", "Type", "Subtype", "Switchtype", "Image", "Options", "Used", "DeviceID", NULL };

		try
		{
			PyObject*	pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, unable to find module for current interpreter.", __func__);
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

			if (PyArg_ParseTupleAndKeywords(args, kwds, "si|siiiiOis", kwlist, &Name, &Unit, &TypeName, &Type, &SubType, &SwitchType, &Image, &Options, &Used, &DeviceID))
			{
				self->pPlugin = pModState->pPlugin;
				self->PluginKey = PyUnicode_FromString(pModState->pPlugin->m_PluginKey.c_str());
				self->HwdID = pModState->pPlugin->m_HwdID;
				if (Name) {
					Py_DECREF(self->Name);
					self->Name = PyUnicode_FromString(Name);
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
					std::string	sTypeName = TypeName;

					self->Type = pTypeGeneral;

					if (sTypeName == "Pressure")					self->SubType = sTypePressure;
					else if (sTypeName == "Percentage")				self->SubType = sTypePercentage;
					else if (sTypeName == "Gas")
					{
						self->Type = pTypeP1Gas;
						self->SubType = sTypeP1Gas;
					}
					else if (sTypeName == "Voltage")				self->SubType = sTypeVoltage;
					else if (sTypeName == "Text")					self->SubType = sTypeTextStatus;
					else if (sTypeName == "Switch")
					{
						self->Type = pTypeGeneralSwitch;
						self->SubType = sSwitchGeneralSwitch;
					}
					else if (sTypeName == "Alert")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("No Alert!");
						self->SubType = sTypeAlert;
					}
					else if (sTypeName == "Current/Ampere")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;0.0;0.0");
						self->Type = pTypeCURRENT;
						self->SubType = sTypeELEC1;
					}
					else if (sTypeName == "Sound Level")			self->SubType = sTypeSoundLevel;
					else if (sTypeName == "Barometer")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("1021.34;0");
						self->SubType = sTypeBaro;
					}
					else if (sTypeName == "Visibility")				self->SubType = sTypeVisibility;
					else if (sTypeName == "Distance")				self->SubType = sTypeDistance;
					else if (sTypeName == "Counter Incremental")	self->SubType = sTypeCounterIncremental;
					else if (sTypeName == "Soil Moisture")			self->SubType = sTypeSoilMoisture;
					else if (sTypeName == "Leaf Wetness")			self->SubType = sTypeLeafWetness;
					else if (sTypeName == "kWh")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0; 0.0");
						self->SubType = sTypeKwh;
					}
					else if (sTypeName == "Current (Single)")		self->SubType = sTypeCurrent;
					else if (sTypeName == "Solar Radiation")		self->SubType = sTypeSolarRadiation;
					else if (sTypeName == "Temperature")
					{
						self->Type = pTypeTEMP;
						self->SubType = sTypeTEMP5;
					}
					else if (sTypeName == "Humidity")
					{
						self->Type = pTypeHUM;
						self->SubType = sTypeHUM1;
					}
					else if (sTypeName == "Temp+Hum")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;50;1");
						self->Type = pTypeTEMP_HUM;
						self->SubType = sTypeTH1;
					}
					else if (sTypeName == "Temp+Hum+Baro")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0.0;50;1;1010;1");
						self->Type = pTypeTEMP_HUM_BARO;
						self->SubType = sTypeTHB1;
					}
					else if (sTypeName == "Wind")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;N;0;0;0;0");
						self->Type = pTypeWIND;
						self->SubType = sTypeWIND1;
					}
					else if (sTypeName == "Rain")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;0");
						self->Type = pTypeRAIN;
						self->SubType = sTypeRAIN3;
					}
					else if (sTypeName == "UV")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;0");
						self->Type = pTypeUV;
						self->SubType = sTypeUV1;
					}
					else if (sTypeName == "Air Quality")
					{
						self->Type = pTypeAirQuality;
						self->SubType = sTypeVoltcraft;
					}
					else if (sTypeName == "Usage")
					{
						self->Type = pTypeUsage;
						self->SubType = sTypeElectric;
					}
					else if (sTypeName == "Illumination")
					{
						self->Type = pTypeLux;
						self->SubType = sTypeLux;
					}
					else if (sTypeName == "Waterflow")				self->SubType = sTypeWaterflow;
					else if (sTypeName == "Wind+Temp+Chill")
					{
						Py_DECREF(self->sValue);
						self->sValue = PyUnicode_FromString("0;N;0;0;0;0");
						self->Type = pTypeWIND;
						self->SubType = sTypeWIND4;
					}
					else if (sTypeName == "Selector Switch")
					{
						if (!Options || !PyDict_Check(Options)) {
							PyDict_Clear(self->Options);
							PyDict_SetItemString(self->Options, "LevelActions", PyUnicode_FromString("|||"));
							PyDict_SetItemString(self->Options, "LevelNames", PyUnicode_FromString("Off|Level1|Level2|Level3"));
							PyDict_SetItemString(self->Options, "LevelOffHidden", PyUnicode_FromString("false"));
							PyDict_SetItemString(self->Options, "SelectorStyle", PyUnicode_FromString("0"));
						}
						self->Type = pTypeGeneralSwitch;
						self->SubType = sSwitchTypeSelector;
						self->SwitchType = 18;
					}
					else if (sTypeName == "Custom")
					{
						self->SubType = sTypeCustom;
						if (!Options || !PyDict_Check(Options)) {
							PyDict_Clear(self->Options);
							PyDict_SetItemString(self->Options, "Custom", PyUnicode_FromString("1"));
						}
					}
				}
				if ((Type != -1) && Type) self->Type = Type;
				if ((SubType != -1) && SubType) self->SubType = SubType;
				if (SwitchType != -1) self->SwitchType = SwitchType;
				if (Image != -1) self->Image = Image;
				if (Used == 1) self->Used = Used;
				if (Options && PyDict_Check(Options) && PyDict_Size(Options) > 0) {
					PyObject *pKey, *pValue;
					Py_ssize_t pos = 0;
					PyDict_Clear(self->Options);
					while(PyDict_Next(Options, &pos, &pKey, &pValue))
					{
						PyObject *pKeyDict = PyUnicode_FromKindAndData(PyUnicode_KIND(pKey), PyUnicode_DATA(pKey), PyUnicode_GET_LENGTH(pKey));
						PyObject *pValueDict = PyUnicode_FromKindAndData(PyUnicode_KIND(pValue), PyUnicode_DATA(pValue), PyUnicode_GET_LENGTH(pValue));
						if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
						{
							_log.Log(LOG_ERROR, "(%s) Failed to initialize Options dictionary for Hardware/Unit combination (%d:%d).", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
							Py_XDECREF(pKeyDict);
							Py_XDECREF(pValueDict);
							break;
						}
						Py_XDECREF(pKeyDict);
						Py_XDECREF(pValueDict);
					}
				}
			}
			else
			{
				CPlugin* pPlugin = NULL;
				if (pModState) pPlugin = pModState->pPlugin;
				_log.Log(LOG_ERROR, "Expected: myVar = Domoticz.Device(Name=\"myDevice\", Unit=0, TypeName=\"\", Type=0, Subtype=0, Switchtype=0, Image=0, Options={}, Used=1)");
				LogPythonException(pPlugin, __func__);
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
			result = m_sql.safe_query("SELECT Unit, ID, Name, nValue, sValue, DeviceID, Type, SubType, SwitchType, LastLevel, CustomImage, SignalLevel, BatteryLevel, LastUpdate, Options FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d) ORDER BY Unit ASC", self->HwdID, self->Unit);
			if (result.size() > 0)
			{
				for (std::vector<std::vector<std::string> >::const_iterator itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
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
							std::map<std::string, std::string> mpOptions = m_sql.BuildDeviceOptions(sd[14], true);
							for (std::map<std::string, std::string>::const_iterator ittOpt = mpOptions.begin(); ittOpt != mpOptions.end(); ++ittOpt)
							{
								PyObject *pKeyDict = PyUnicode_FromString(ittOpt->first.c_str());
								PyObject *pValueDict =  PyUnicode_FromString(ittOpt->second.c_str());
								if (PyDict_SetItem(self->Options, pKeyDict, pValueDict) == -1)
								{
									_log.Log(LOG_ERROR, "(%s) Failed to refresh Options dictionary for Hardware/Unit combination (%d:%d).", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
									Py_DECREF(pKeyDict);
									Py_DECREF(pValueDict);
									break;
								}
								Py_DECREF(pKeyDict);
								Py_DECREF(pValueDict);
							}
						}
					}
				}
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device refresh failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CDevice_insert(CDevice* self)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			std::string	sDeviceID = PyUnicode_AsUTF8(self->DeviceID);
			if (self->ID == -1)
			{
				if (self->pPlugin->m_bDebug)
				{
					_log.Log(LOG_NORM, "(%s) Creating device '%s'.", self->pPlugin->Name.c_str(), sName.c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
				if (result.size() == 0)
				{
					std::string	sValue = PyUnicode_AsUTF8(self->sValue);
					std::string	sLongName = self->pPlugin->Name + " - " + sName;
					if ((self->SubType == sTypeCustom) && (PyDict_Size(self->Options) > 0))
					{
						PyObject *pValueDict = PyDict_GetItemString(self->Options, "Custom");
						std::string sOptionValue;
						if (pValueDict == NULL)
							sOptionValue = "";
						else
							sOptionValue = PyUnicode_AsUTF8(pValueDict);

						m_sql.safe_query(
							"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage, Options) "
							"VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d, '%q')",
							self->HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used, sLongName.c_str(), sValue.c_str(), self->Image, sOptionValue.c_str());
					}
					else
					{
						m_sql.safe_query(
							"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue, CustomImage) "
							"VALUES (%d, '%q', %d, %d, %d, %d, %d, 12, 255, '%q', 0, '%q', %d)",
							self->HwdID, sDeviceID.c_str(), self->Unit, self->Type, self->SubType, self->SwitchType, self->Used, sLongName.c_str(), sValue.c_str(), self->Image);
					}

					result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
					if (result.size())
					{
						self->ID = atoi(result[0][0].c_str());

						PyObject*	pKey = PyLong_FromLong(self->Unit);
						if (PyDict_SetItem((PyObject*)self->pPlugin->m_DeviceDict, pKey, (PyObject*)self) == -1)
						{
							_log.Log(LOG_ERROR, "(%s) failed to add unit '%d' to device dictionary.", self->pPlugin->Name.c_str(), self->Unit);
							Py_INCREF(Py_None);
							return Py_None;
						}

						// Device successfully created, now set the options when supplied
						if ((self->SubType != sTypeCustom) && (PyDict_Size(self->Options) > 0))
						{
							PyObject *pKeyDict, *pValueDict;
							Py_ssize_t pos = 0;
							std::map<std::string, std::string> mpOptions;
							while(PyDict_Next(self->Options, &pos, &pKeyDict, &pValueDict)) {
								std::string sOptionName = PyUnicode_AsUTF8(pKeyDict);
								std::string sOptionValue = PyUnicode_AsUTF8(pValueDict);
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
						_log.Log(LOG_ERROR, "(%s) Device creation failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Device creation failed, Hardware/Unit combination (%d:%d) already exists in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Device creation failed, '%s' already exists in Domoticz with Device ID '%d'.", self->pPlugin->Name.c_str(), sName.c_str(), self->ID);
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device creation failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CDevice_update(CDevice *self, PyObject *args, PyObject *kwds)
	{
		if (self->pPlugin)
		{
			int			nValue = self->nValue;
			char*		sValue = NULL;
			int			iSignalLevel = self->SignalLevel;
			int			iBatteryLevel = self->BatteryLevel;
			int			iImage = self->Image;
			int			iTimedOut = self->TimedOut;
			PyObject*	pOptionsDict = NULL;
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			std::string	sDeviceID = PyUnicode_AsUTF8(self->DeviceID);
			static char *kwlist[] = { "nValue", "sValue", "Image", "SignalLevel", "BatteryLevel", "Options", "TimedOut", NULL };

			if (!PyArg_ParseTupleAndKeywords(args, kwds, "is|iiiOi", kwlist, &nValue, &sValue, &iImage, &iSignalLevel, &iBatteryLevel, &pOptionsDict, &iTimedOut))
			{
				_log.Log(LOG_ERROR, "(%s) %s: Failed to parse parameters: 'nValue', 'sValue', 'SignalLevel', 'BatteryLevel', 'Options' or 'TimedOut' expected.", __func__, sName.c_str());
				LogPythonException(self->pPlugin, __func__);
				Py_INCREF(Py_None);
				return Py_None;
			}

			if (self->pPlugin->m_bDebug)
			{
				_log.Log(LOG_NORM, "(%s) Updating device from %d:'%s' to have values %d:'%s'.", sName.c_str(), self->nValue, PyUnicode_AsUTF8(self->sValue), nValue, sValue);
			}
			m_sql.UpdateValue(self->HwdID, sDeviceID.c_str(), (const unsigned char)self->Unit, (const unsigned char)self->Type, (const unsigned char)self->SubType, iSignalLevel, iBatteryLevel, nValue, std::string(sValue).c_str(), sName, true);

			// Notify MQTT and various push mechanisms
			m_mainworker.sOnDeviceReceived(self->pPlugin->m_HwdID, self->ID, self->pPlugin->Name, NULL);

			// Image change
			if (iImage != self->Image)
			{
				time_t now = time(0);
				struct tm ltime;
				localtime_r(&now, &ltime);
				m_sql.safe_query("UPDATE DeviceStatus SET CustomImage=%d, LastUpdate='%04d-%02d-%02d %02d:%02d:%02d' WHERE (HardwareID==%d) and (Unit==%d)",
					iImage, ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday, ltime.tm_hour, ltime.tm_min, ltime.tm_sec, self->HwdID, self->Unit);
			}

			if ((self->SubType != sTypeCustom) && (pOptionsDict != NULL))
			{
				// Options provided, assume change
				PyObject *pKeyDict, *pValueDict;
				Py_ssize_t pos = 0;
				std::map<std::string, std::string> mpOptions;
				while(PyDict_Next(pOptionsDict, &pos, &pKeyDict, &pValueDict)) {
					std::string sOptionName = PyUnicode_AsUTF8(pKeyDict);
					std::string sOptionValue = PyUnicode_AsUTF8(pValueDict);
					mpOptions.insert(std::pair<std::string, std::string>(sOptionName, sOptionValue));
				}
				m_sql.SetDeviceOptions(self->ID, mpOptions);
			}

			// TimedOut change
			if (iTimedOut != self->TimedOut)
			{
				self->TimedOut = iTimedOut;
			}

			CDevice_refresh(self);
		}
		else
		{
			_log.Log(LOG_ERROR, "Device update failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CDevice_delete(CDevice* self)
	{
		if (self->pPlugin)
		{
			std::string	sName = PyUnicode_AsUTF8(self->Name);
			if (self->ID != -1)
			{
				if (self->pPlugin->m_bDebug)
				{
					_log.Log(LOG_NORM, "(%s) Deleting device '%s'.", self->pPlugin->Name.c_str(), sName.c_str());
				}

				std::vector<std::vector<std::string> > result;
				result = m_sql.safe_query("SELECT Name FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);
				if (result.size() != 0)
				{
					m_sql.safe_query("DELETE FROM DeviceStatus WHERE (HardwareID==%d) AND (Unit==%d)", self->HwdID, self->Unit);

					PyObject*	pKey = PyLong_FromLong(self->Unit);
					if (PyDict_DelItem((PyObject*)self->pPlugin->m_DeviceDict, pKey) == -1)
					{
						_log.Log(LOG_ERROR, "(%s) failed to delete unit '%d' from device dictionary.", self->pPlugin->Name.c_str(), self->Unit);
						Py_INCREF(Py_None);
						return Py_None;
					}
				}
				else
				{
					_log.Log(LOG_ERROR, "(%s) Device deletion failed, Hardware/Unit combination (%d:%d) not found in Domoticz.", self->pPlugin->Name.c_str(), self->HwdID, self->Unit);
				}
			}
			else
			{
				_log.Log(LOG_ERROR, "(%s) Device deletion failed, '%s' does not represent a device in Domoticz.", self->pPlugin->Name.c_str(), sName.c_str());
			}
		}
		else
		{
			_log.Log(LOG_ERROR, "Device deletion failed, Device object is not associated with a plugin.");
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject* CDevice_str(CDevice* self)
	{
		PyObject*	pRetVal = PyUnicode_FromFormat("ID: %d, Name: '%U', nValue: %d, sValue: '%U'", self->ID, self->Name, self->nValue, self->sValue);
		return pRetVal;
	}

	void CConnection_dealloc(CConnection * self)
	{
		if (self->pPlugin && self->pPlugin->m_bDebug)
		{
			_log.Log(LOG_NORM, "(%s) Deallocating connection object '%s' (%s:%s).", self->pPlugin->Name.c_str(), PyUnicode_AsUTF8(self->Name), PyUnicode_AsUTF8(self->Address), PyUnicode_AsUTF8(self->Port));
		}

		Py_XDECREF(self->Address);
		Py_XDECREF(self->Port);
		Py_XDECREF(self->LastSeen);
		Py_XDECREF(self->Transport);
		Py_XDECREF(self->Protocol);

		if (self->pTransport)
		{
			delete self->pTransport;
			self->pTransport = NULL;
		}
		if (self->pProtocol)
		{
			delete self->pProtocol;
			self->pProtocol = NULL;
		}

		Py_TYPE(self)->tp_free((PyObject*)self);
	}

	PyObject * CConnection_new(PyTypeObject * type, PyObject * args, PyObject * kwds)
	{
		CConnection *self = NULL;
		if ((CConnection *)type->tp_alloc)
		{
			self = (CConnection *)type->tp_alloc(type, 0);
		}
		else
		{
			_log.Log(LOG_ERROR, "(%s) CConnection Type is not ready.", self->pPlugin->Name.c_str());
		}

		try
		{
			if (self == NULL) {
				_log.Log(LOG_ERROR, "%s: Self is NULL.", __func__);
			}
			else {
				self->Name = PyUnicode_FromString("");
				if (self->Name == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Address = PyUnicode_FromString("");
				if (self->Address == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Port = PyUnicode_FromString("");
				if (self->Port == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->LastSeen = PyUnicode_FromString("");
				if (self->LastSeen == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Transport = PyUnicode_FromString("");
				if (self->Transport == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->Protocol = PyUnicode_FromString("None");
				if (self->Protocol == NULL) {
					Py_DECREF(self);
					return NULL;
				}
				self->pPlugin = NULL;
				self->pTransport = NULL;
				self->pProtocol = NULL;
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
		char*		pName = NULL;
		char*		pTransport = NULL;
		char*		pProtocol = NULL;
		char*		pAddress = NULL;
		char*		pPort = NULL;
		int			iBaud = -1;
		static char *kwlist[] = { "Name", "Transport", "Protocol", "Address", "Port", "Baud", NULL };

		try
		{
			PyObject*	pModule = PyState_FindModule(&DomoticzModuleDef);
			if (!pModule)
			{
				_log.Log(LOG_ERROR, "CPlugin:%s, unable to find module for current interpreter.", __func__);
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

			if (PyArg_ParseTupleAndKeywords(args, kwds, "ss|sssi", kwlist, &pName, &pTransport, &pProtocol, &pAddress, &pPort, &iBaud))
			{
				self->pPlugin = pModState->pPlugin;
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
					ProtocolDirective*	Message = new ProtocolDirective(self->pPlugin, (PyObject*)self);
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
			else
			{
				CPlugin* pPlugin = NULL;
				if (pModState) pPlugin = pModState->pPlugin;
				_log.Log(LOG_ERROR, "Expected: myVar = Domoticz.Connection(Name=\"<Name>\", Transport=\"<Transport>\", Protocol=\"<Protocol>\", Address=\"<IP-Address>\", Port=\"<Port>\", Baud=0)");
				LogPythonException(pPlugin, __func__);
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

	PyObject * CConnection_connect(CConnection * self)
	{
		Py_INCREF(Py_None);

		if (!self->pPlugin)
		{
			_log.Log(LOG_ERROR, "%s:, illegal operation, Plugin has not started yet.", __func__);
			return Py_None;
		}

		//	Add connect command to message queue unless already connected
		if (self->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, connect request from '%s' ignored. Plugin is stopping.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		if (self->pTransport && self->pTransport->IsConnecting())
		{
			_log.Log(LOG_ERROR, "%s, connect request from '%s' ignored. Transport is connecting.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		if (self->pTransport && self->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "%s, connect request from '%s' ignored. Transport is connected.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		ConnectDirective*	Message = new ConnectDirective(self->pPlugin, (PyObject*)self);
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);

		return Py_None;
	}

	PyObject * CConnection_listen(CConnection * self)
	{
		Py_INCREF(Py_None);

		if (!self->pPlugin)
		{
			_log.Log(LOG_ERROR, "%s:, illegal operation, Plugin has not started yet.", __func__);
			return Py_None;
		}

		//	Add connect command to message queue unless already connected
		if (self->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, listen request from '%s' ignored. Plugin is stopping.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		if (self->pTransport && self->pTransport->IsConnecting())
		{
			_log.Log(LOG_ERROR, "%s, listen request from '%s' ignored. Transport is connecting.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		if (self->pTransport && self->pTransport->IsConnected())
		{
			_log.Log(LOG_ERROR, "%s, listen request from '%s' ignored. Transport is connected.", __func__, self->pPlugin->Name.c_str());
			return Py_None;
		}

		ListenDirective*	Message = new ListenDirective(self->pPlugin, (PyObject*)self);
		boost::lock_guard<boost::mutex> l(PluginMutex);
		PluginMessageQueue.push(Message);

		return Py_None;
	}

	PyObject * CConnection_send(CConnection * self, PyObject * args, PyObject * kwds)
	{
		if (!self->pPlugin)
		{
			_log.Log(LOG_ERROR, "%s:, illegal operation, Plugin has not started yet.", __func__);
		}
		else if (self->pPlugin->m_stoprequested)
		{
			_log.Log(LOG_NORM, "%s, send request from '%s' ignored. Plugin is stopping.", __func__, self->pPlugin->Name.c_str());
		}
		else
		{
			PyObject*	pData = NULL;
			int			iDelay = 0;
			static char *kwlist[] = { "Message", "Delay", NULL };
			if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist, &pData, &iDelay))
			{
				_log.Log(LOG_ERROR, "(%s) failed to parse parameters, Message or Message, Delay expected.", self->pPlugin->Name.c_str());
				LogPythonException(self->pPlugin, std::string(__func__));
			}
			else
			{
				//	Add start command to message queue
				WriteDirective*	Message = new WriteDirective(self->pPlugin, (PyObject*)self, pData, iDelay);
				{
					boost::lock_guard<boost::mutex> l(PluginMutex);
					PluginMessageQueue.push(Message);
				}
			}
		}

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject * CConnection_disconnect(CConnection * self)
	{
		if (self->pTransport)
		{
			if (self->pTransport->IsConnecting() || self->pTransport->IsConnected())
			{
				DisconnectDirective*	Message = new DisconnectDirective(self->pPlugin, (PyObject*)self);
				boost::lock_guard<boost::mutex> l(PluginMutex);
				PluginMessageQueue.push(Message);
			}
			else
				_log.Log(LOG_ERROR, "%s, disconnection request from '%s' ignored. Transport is not connecting or connected.", __func__, self->pPlugin->Name.c_str());
		}
		else
			_log.Log(LOG_ERROR, "%s, disconnection request from '%s' ignored. Transport does not exist.", __func__, self->pPlugin->Name.c_str());

		Py_INCREF(Py_None);
		return Py_None;
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

		Py_INCREF(Py_None);
		return Py_None;
	}

	PyObject * CConnection_str(CConnection * self)
	{
		time_t	tLastSeen = self->pTransport->LastSeen();
		struct tm ltime;
		localtime_r(&tLastSeen, &ltime);
		char date[32];
		strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &ltime);
		PyObject*	pRetVal = PyUnicode_FromFormat("Name: '%U', Transport: '%U', Protocol: '%U', Address: '%U', Port: '%U', Baud: %d, Bytes: %d, Connected: %s, Last Seen: %s",
			self->Name, self->Transport, self->Protocol, self->Address, self->Port, self->Baud,
			(self->pTransport ? self->pTransport->TotalBytes() : -1),
			(self->pTransport ? (self->pTransport->IsConnected() ? "True" : "False") : "False"), date);
		return pRetVal;
	}

}
#endif
