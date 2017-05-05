import DomoticzEvents as DE

if DE.is_daytime:
    DE.Log("Python: It's daytime!")

if DE.is_nighttime:
    DE.Log("Python: It's nighttime!")

DE.Log("Python: Sunrise in minutes: " + str(DE.sunrise_in_minutes))
DE.Log("Python: Sunset in minutes: " + str(DE.sunset_in_minutes))
DE.Log("Python: Minutes since midnight: " + str(DE.minutes_since_midnight))
