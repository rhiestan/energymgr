cmake_minimum_required(VERSION 3.10)

project(energymgr)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Network Mqtt Sql)

qt_standard_project_setup()

SET(CMAKE_AUTOMOC ON)

#add_executable(energymgr energymgrmain.cpp)
qt_add_executable(energymgr
    energymgrmain.cpp
	CommandRunner.cpp
	CommandRunner.h
	Config.cpp
	Config.h
	MQReader.cpp
	MQReader.h
	ValueDBStorage.cpp
	ValueDBStorage.h
	FroniusReader.cpp
	FroniusReader.h
	OpenHABSystemStatusReader.cpp
	OpenHABSystemStatusReader.h
	ReadInfluxDBHistoryValue.cpp
	ReadInfluxDBHistoryValue.h
	Logger.cpp
	Logger.h
	EnergyManager.cpp
	EnergyManager.h
	EnergyValue.cpp
	EnergyValue.h
	SendValueToOpenHAB.cpp
	SendValueToOpenHAB.h
)
target_link_libraries(energymgr PRIVATE
	Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Mqtt Qt6::Sql
)
