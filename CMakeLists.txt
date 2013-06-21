CMAKE_MINIMUM_REQUIRED(VERSION 2.8.10)

PROJECT(LadybugVelodyne)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

SET(LadybugVelodyne_SOURCES 
    LadybugVelodyne.cpp
    main.cpp
    ViewerWidget.cpp
)
SET(LadybugVelodyne_HEADERS 
    LadybugVelodyne.h
    ViewerWidget.h
)
SET(LadybugVelodyne_FORMS LadybugVelodyne.ui) 
SET(LadybugVelodyne_RESOURCES LadybugVelodyne.qrc)

Qt5_WRAP_CPP(LadybugVelodyne_HEADERS_MOC ${LadybugVelodyne_HEADERS})
Qt5_WRAP_UI(LadybugVelodyne_FORMS_HEADERS ${LadybugVelodyne_FORMS})
Qt5_ADD_RESOURCES(LadybugVelodyne_RESOURCES_RCC ${LadybugVelodyne_RESOURCES})

INCLUDE_DIRECTORIES(
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
)

IF(UNIX)
    ADD_DEFINITIONS(-fPIC)
ENDIF(UNIX)

ADD_EXECUTABLE(LadybugVelodyne
    ${LadybugVelodyne_SOURCES} 
    ${LadybugVelodyne_FORMS_HEADERS}
    ${LadybugVelodyne_RESOURCES_RCC}
)
TARGET_LINK_LIBRARIES(LadybugVelodyne
    Qt5::Gui
    Qt5::OpenGL
    ${OPENGL_LIBRARIES}
)
