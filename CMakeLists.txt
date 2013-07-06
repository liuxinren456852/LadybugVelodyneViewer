CMAKE_MINIMUM_REQUIRED(VERSION 2.8.10)

PROJECT(LadybugVelodyne)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)

SET(LadybugVelodyne_SOURCES 
    LadybugVelodyne.cpp
    main.cpp
    ViewerWidget.cpp
)
SET(LadybugVelodyne_HEADERS 
    LadybugVelodyne.h
    ViewerWidget.h
	glew.h
	wglew.h
)
SET(LadybugVelodyne_FORMS LadybugVelodyne.ui) 
SET(LadybugVelodyne_RESOURCES LadybugVelodyne.qrc)

Qt5_WRAP_CPP(LadybugVelodyne_HEADERS_MOC ${LadybugVelodyne_HEADERS})
Qt5_WRAP_UI(LadybugVelodyne_FORMS_HEADERS ${LadybugVelodyne_FORMS})
Qt5_ADD_RESOURCES(LadybugVelodyne_RESOURCES_RCC ${LadybugVelodyne_RESOURCES})

INCLUDE_DIRECTORIES(
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

IF(UNIX)
    ADD_DEFINITIONS(-fPIC)
ENDIF(UNIX)

SET(LadybugVelodyne_GLSL_SHADERS
    shaders/panorama_vertex.glsl
    shaders/panorama_fragment.glsl
    shaders/points_vertex.glsl
    shaders/points_fragment.glsl
)

SET(LadybugVelodyne_CALIBRATIONS
	calibrations/velodyne.calib
)

FOREACH (shader ${LadybugVelodyne_GLSL_SHADERS})
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${shader}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${shader} ${CMAKE_CURRENT_BINARY_DIR}/${shader}
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${shader})
ENDFOREACH(shader)

FOREACH (calibration ${LadybugVelodyne_CALIBRATIONS})
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${calibration}
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${calibration} ${CMAKE_CURRENT_BINARY_DIR}/${calibration}
		MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${calibration})
ENDFOREACH(calibration)

INCLUDE_DIRECTORIES("C:\\Program Files\\Point Grey Research\\Ladybug\\include")
LINK_DIRECTORIES("C:\\Program Files\\Point Grey Research\\Ladybug\\lib64" ".")

ADD_EXECUTABLE(LadybugVelodyne
    ${LadybugVelodyne_SOURCES} 
    ${LadybugVelodyne_FORMS_HEADERS}
    ${LadybugVelodyne_RESOURCES_RCC}
	${LadybugVelodyne_GLSL_SHADERS}
	${LadybugVelodyne_CALIBRATIONS}
)
TARGET_LINK_LIBRARIES(LadybugVelodyne
	Qt5::Widgets
    Qt5::Gui
    ${OPENGL_LIBRARIES}
	ladybug.lib
	glew32.lib
)
