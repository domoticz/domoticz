from distutils.core import setup, Extension
setup(name="DomoticzEvents", version="1.0",
      ext_modules=[Extension("DomoticzEvents", ["EventsPythonDevice.cpp"], define_macros=[('BUILD_MODULE ', None)])])
