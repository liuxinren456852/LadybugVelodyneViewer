CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(LadybugVelodyne)
FIND_PACKAGE(Qt4 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

SET(QT_USE_QTOPENGL TRUE)

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

QT4_WRAP_CPP(LadybugVelodyne_HEADERS_MOC ${LadybugVelodyne_HEADERS})
QT4_WRAP_UI(LadybugVelodyne_FORMS_HEADERS ${LadybugVelodyne_FORMS})
QT4_ADD_RESOURCES(LadybugVelodyne_RESOURCES_RCC ${LadybugVelodyne_RESOURCES})

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFITIIONS})

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

ADD_EXECUTABLE(LadybugVelodyne 
    ${LadybugVelodyne_SOURCES} 
    ${LadybugVelodyne_HEADERS_MOC}
    ${LadybugVelodyne_FORMS_HEADERS}
    ${LadybugVelodyne_RESOURCES_RCC}
)
TARGET_LINK_LIBRARIES(LadybugVelodyne 
    ${QT_LIBRARIES}
    ${OPENGL_LIBRARIES}
)
