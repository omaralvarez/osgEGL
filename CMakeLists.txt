cmake_minimum_required(VERSION 2.8.8)
SET(PROJECT_NAME osgEGL)
PROJECT(${PROJECT_NAME})

LIST( APPEND CMAKE_MODULE_PATH "${${PROJECT_NAME}_SOURCE_DIR}/CMakeModules" )

find_package(EGL REQUIRED)
FIND_PACKAGE (OpenSceneGraph REQUIRED COMPONENTS osgUtil osgDB osgText osgGA osgFX osgSim osgViewer CONFIG)

SET(SOURCES
    main.cpp
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCES})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${EGL_LIBRARY} osg3::osg osg3::osgUtil osg3::osgDB osg3::osgText osg3::osgGA osg3::osgFX osg3::osgSim osg3::osgViewer)

