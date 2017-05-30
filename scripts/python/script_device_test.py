import DomoticzEvents as DE

DE.Log("Python: Changed: " + DE.changed_device.Describe())

if DE.changed_device_name == "Test":
    if DE.Devices["Test_Target"].n_value_string == "On":
        DE.Command("Test_Target", "Off")

    if DE.Devices["Test_Target"].n_value_string == "Off":
        DE.Command("Test_Target", "On")

DE.Log("Python: Number of user_variables: " + str(len(DE.user_variables)))

# All user_variables are treated as strings, convert as necessary
for key, value in DE.user_variables.items():
    DE.Log("Python: User-variable '{0}' has value: {1}".format(key, value))

# Description of Device_object should go here...
