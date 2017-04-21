INCLUDES= -I. -I./src -I/usr/include/python3.4

python:	EventsPythonDevice.cpp EventsPythonDevice.h
	python3 setup.py build

clean:
	rm -rf *.o lux build/* obj/*
