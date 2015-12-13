set(CMAKE_OSX_ARCHITECTURES x86_64)
project(RocketEditor)

cmake_minimum_required(VERSION 2.8)

set(RKT_EXE_NAME "RocketEditor")

set(VERSION_MAJOR "1")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

if(APPLE)
	add_definitions(-DMACOSX)
	add_definitions(-DEMGUI_MACOSX)
	add_compile_options(-Wall)
	add_compile_options(-Wno-format-security)
	add_compile_options(-Wno-deprecated-declarations)
endif()

##############################################################################
# MXML
file(GLOB MXML_SRCS
    external/mxml/*.c
)
add_library(mxml ${MXML_SRCS})
target_include_directories(mxml PUBLIC external/mxml)
target_compile_definitions(mxml PUBLIC _THREAD_SAFE _REENTRANT)

##############################################################################
# SYNC
file(GLOB SYNC_SRCS
    ../lib/*.c
)
add_library(sync ${SYNC_SRCS})
target_include_directories(sync PUBLIC ../lib)

##############################################################################
# EMGUI
file(GLOB_RECURSE EMGUI_SRCS
    emgui/src/*.c
    emgui/src/*.h
)
add_library(emgui ${EMGUI_SRCS})
target_include_directories(emgui PUBLIC emgui/src emgui/include)
target_compile_definitions(emgui PUBLIC _THREAD_SAFE _REENTRANT)
target_compile_options(emgui PUBLIC -Werror -pedantic-errors)

##############################################################################
# EDITOR
file(GLOB PROJECT_SRCS
    src/*.c
    src/*.m
    src/*.h
)
if (APPLE)
    file(GLOB PROJECT_PLATFORM_SRCS
        src/macosx/*.c
        src/macosx/*.m
        src/macosx/*.h
    )
endif()
file(GLOB RESOURCES_DATA
    data/macosx/icon.icns
)
source_group("Data" FILES ${RESOURCES_DATA})

if (APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)

	find_library(COCOA_FRAMEWORK Cocoa)
    find_library(OPENGL_FRAMEWORK OpenGL)
    find_library(CARBON_FRAMEWORK Carbon)

    mark_as_advanced(COCOA_FRAMEWORK OPENGL_FRAMEWORK CARBON_FRAMEWORK)

    set(PLATFORM_LIBS ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK} ${CARBON_FRAMEWORK})

	# Define some settings for the Bundle
    set(MACOSX_BUNDLE_BUNDLE_NAME ${RKT_EXE_NAME})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tbl.rocketeditor")
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(MACOSX_BUNDLE_INFO_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH},Copyright © 2016 GNU Rocket Contributors")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2012-2016 GNU Rocket Contributors. All rights reserved.")

	set_source_files_properties(${RESOURCES_DATA} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    set(PROJECT_SRCS ${GUI_TYPE} ${PROJECT_SRCS} ${RESOURCES_DATA})

endif (APPLE)
add_executable(${RKT_EXE_NAME} ${PROJECT_SRCS} ${PROJECT_PLATFORM_SRCS} data/macosx/appnib.xib)
if (APPLE)
    set_target_properties(${RKT_EXE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/MacOSXBundleInfo.plist.in)
endif ()
target_include_directories(${RKT_EXE_NAME} PUBLIC . src emgui/include external/mxml)
target_compile_options(${RKT_EXE_NAME} PUBLIC -Werror -pedantic-errors)
target_link_libraries(${RKT_EXE_NAME} sync mxml emgui ${PLATFORM_LIBS})

# compile the nibs
if (APPLE)
    find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
    if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
        message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
    endif()

    # Make sure the 'Resources' Directory is correctly created before we build
    #add_custom_command (TARGET ${RKT_EXE_NAME} PRE_BUILD COMMAND echo ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${RKT_EXE_NAME}.app/Contents/Resources/appnib.nib)

    # Compile the .xib files using the 'ibtool' program with the destination being the app package
    add_custom_command (TARGET ${RKT_EXE_NAME} POST_BUILD 
                          COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text 
                                --compile ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${RKT_EXE_NAME}.app/Contents/Resources/appnib.nib 
                                ${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/appnib.xib
                          COMMENT "Compiling appnib.xib")
endif (APPLE)