project(RocketEditor)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_OSX_ARCHITECTURES x86_64)

set(RKT_EXE_NAME "RocketEditor")

set(VERSION_MAJOR "1")
set(VERSION_MINOR "1")
set(VERSION_PATCH "0")

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

if (APPLE)
	add_definitions(-DMACOSX -DEMGUI_MACOSX)
	add_compile_options(-Wall -Wno-format-security -Wno-deprecated-declarations)
elseif (UNIX)
	add_definitions(-DEMGUI_UNIX)
	add_compile_options(-Wall -Wno-format-security -Wno-deprecated-declarations)
elseif (MSVC)
	add_definitions(-DEMGUI_WINDOWS -DUNICODE -D_UNICODE -DWIN32)
    add_compile_options(/W4 /FS /wd4996 /wd4389)
endif ()

##############################################################################
# MXML
file(GLOB RKT_MXML_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/external/mxml/*.c
)
add_library(rkt_mxml ${RKT_MXML_SRCS})
target_include_directories(rkt_mxml PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/mxml)
if (APPLE)
	target_compile_definitions(rkt_mxml PUBLIC -D_THREAD_SAFE -D_REENTRANT)
	target_compile_options(rkt_mxml PUBLIC -Wall)
elseif (UNIX)
	target_compile_definitions(rkt_mxml PUBLIC -D_THREAD_SAFE -D_REENTRANT)
	target_compile_options(rkt_mxml PUBLIC -Wall)
elseif (MSVC)
	set_target_properties(rkt_mxml PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif ()
set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/external/mxml)
set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} rkt_mxml)

##############################################################################
# SYNC
file(GLOB RKT_SYNC_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/external/rocket/lib/*.c
)
add_library(rkt_sync ${RKT_SYNC_SRCS})
target_include_directories(rkt_sync PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/rocket/lib)
set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/external/rocket/lib)
set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} rkt_sync)

##############################################################################
# TINYCTHREAD
file(GLOB RKT_TINYCTHREAD_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/external/tinycthread/*.c
)
add_library(rkt_tinycthread ${RKT_TINYCTHREAD_SRCS})
target_include_directories(rkt_tinycthread PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/tinycthread)
if (APPLE)
	target_compile_options(rkt_tinycthread PUBLIC -Wall)
elseif (UNIX)
	target_compile_options(rkt_tinycthread PUBLIC -Wall)
elseif (MSVC)
	set_target_properties(rkt_tinycthread PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif ()
set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/external/tinycthread)
set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} rkt_tinycthread)

##############################################################################
# BASS
set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${CMAKE_SOURCE_DIR}/external/bass)
if (APPLE)
	set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} ${CMAKE_SOURCE_DIR}/external/bass/mac/libbass.dylib)
elseif (UNIX)
	set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} ${CMAKE_SOURCE_DIR}/external/bass/linux/libbass.so)
elseif (MSVC)
	set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} ${CMAKE_SOURCE_DIR}/external/bass/win32/bass.lib)
endif ()

##############################################################################
# EMGUI
file(GLOB_RECURSE RKT_EMGUI_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/emgui/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/emgui/src/*.h
)
add_library(rkt_emgui ${RKT_EMGUI_SRCS})
target_include_directories(rkt_emgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/emgui/src ${CMAKE_CURRENT_SOURCE_DIR}/emgui/include)
if (APPLE)
	target_compile_definitions(rkt_emgui PUBLIC -DEMGUI_MACOSX)
	target_compile_options(rkt_emgui PUBLIC -Werror -pedantic-errors -Wall -Wno-format-security)
elseif (UNIX)
	target_compile_definitions(rkt_emgui PUBLIC -DEMGUI_UNIX)
	target_compile_options(rkt_emgui PUBLIC -Werror -pedantic-errors -Wall -Wno-format-security)
elseif (MSVC)
	target_compile_definitions(rkt_emgui PUBLIC -DEMGUI_WINDOWS)
	set_target_properties(rkt_emgui PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif ()
set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/emgui/include)
set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} rkt_emgui)

##############################################################################
# ROCKET EDITOR
file(GLOB RKT_PROJECT_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.m
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
if (APPLE)
	file(GLOB RKT_PLATFORM_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/src/macosx/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/macosx/*.m
		${CMAKE_CURRENT_SOURCE_DIR}/src/macosx/*.h
	)

	file(GLOB RKT_RESOURCES_DATA
	    ${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/icon.icns
	)
	source_group("Data" FILES ${RKT_RESOURCES_DATA})
elseif (UNIX)
	file(GLOB RKT_PLATFORM_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/src/linux/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/linux/*.h
	)
elseif (MSVC)
	file(GLOB RKT_PLATFORM_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.c
		${CMAKE_CURRENT_SOURCE_DIR}/src/windows/*.h
	)

	file(GLOB RKT_RESOURCES_DATA
	    ${CMAKE_CURRENT_SOURCE_DIR}/data/windows/appicon.ico
	    ${CMAKE_CURRENT_SOURCE_DIR}/data/windows/editor.rc
	)
	source_group("Data" FILES ${RKT_RESOURCES_DATA})
endif ()

set(RKT_PROJECT_SRCS ${RKT_PROJECT_SRCS} ${RKT_PLATFORM_SRCS} ${RKT_RESOURCES_DATA})

set(RKT_PROJECT_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/src ${RKT_PROJECT_INCLUDES})

##############################################################################
#### APPLE BUNDLE, RESSOURCES AND DYNAMIC LIBS
if (APPLE)
	set(GUI_TYPE MACOSX_BUNDLE)

	# Define some settings for the Bundle
	set(MACOSX_BUNDLE_BUNDLE_NAME ${RKT_EXE_NAME})
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.tbl.rocketeditor")
	set(MACOSX_BUNDLE_ICON_FILE icon.icns)
	set(MACOSX_BUNDLE_INFO_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH},Copyright © 2016 GNU Rocket Contributors")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
	set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2012-2016 GNU Rocket Contributors. All rights reserved.")

	set_source_files_properties(${RKT_RESOURCES_DATA} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

	set(OSX_LIB_FILES ${CMAKE_SOURCE_DIR}/external/bass/mac/libbass.dylib)
	set_source_files_properties(${OSX_LIB_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)
	set(RKT_PROJECT_SRCS ${RKT_PROJECT_SRCS} ${OSX_LIB_FILES})

	set(RKT_PROJECT_SRCS ${GUI_TYPE} ${RKT_PROJECT_SRCS} ${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/appnib.xib)

	find_library(COCOA_FRAMEWORK Cocoa)
	find_library(OPENGL_FRAMEWORK OpenGL)
	find_library(CARBON_FRAMEWORK Carbon)
	find_library(COREAUDIO_FRAMEWORK CoreAudio)
	mark_as_advanced(COCOA_FRAMEWORK OPENGL_FRAMEWORK CARBON_FRAMEWORK COREAUDIO_FRAMEWORK)
	set(PLATFORM_LIBS ${COCOA_FRAMEWORK} ${OPENGL_FRAMEWORK} ${CARBON_FRAMEWORK} ${COREAUDIO_FRAMEWORK})
elseif (UNIX)
	find_package(SDL REQUIRED)
	if (NOT SDL_FOUND)
		message(FATAL_ERROR "SDL not found!")
	endif ()

	set(PLATFORM_LIBS ${SDL_LIBRARY} SDLmain GL m)
	set(RKT_PROJECT_INCLUDES ${RKT_PROJECT_INCLUDES} ${SDL_INCLUDE_DIR})
elseif (MSVC)
	set(PLATFORM_LIBS wsock32 opengl32 glu32 kernel32 user32 gdi32 Comdlg32 Advapi32)
endif ()
set(RKT_PROJECT_LIBS ${RKT_PROJECT_LIBS} ${PLATFORM_LIBS})
##############################################################################

add_executable(${RKT_EXE_NAME} ${RKT_PROJECT_SRCS})
if (APPLE)
	set_target_properties(${RKT_EXE_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/MacOSXBundleInfo.plist.in)
elseif (MSVC)
	set_target_properties(${RKT_EXE_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
endif ()
if (APPLE)
	target_compile_definitions(${RKT_EXE_NAME} PUBLIC -DEMGUI_MACOSX)
	target_compile_options(${RKT_EXE_NAME} PUBLIC -Werror -pedantic-errors -Wall -Wno-format-security)
elseif (UNIX)
	target_compile_definitions(${RKT_EXE_NAME} PUBLIC -DEMGUI_UNIX)
	target_compile_options(${RKT_EXE_NAME} PUBLIC -Werror -pedantic-errors -Wall -Wno-format-security)
elseif (MSVC)
	target_compile_definitions(${RKT_EXE_NAME} PUBLIC -DEMGUI_WIN32)
endif ()
target_include_directories(${RKT_EXE_NAME} PUBLIC ${RKT_PROJECT_INCLUDES})
target_link_libraries(${RKT_EXE_NAME} ${RKT_PROJECT_LIBS})

# compile the nibs
if (APPLE)
	find_program(IBTOOL ibtool HINTS "/usr/bin" "${OSX_DEVELOPER_ROOT}/usr/bin")
	if (${IBTOOL} STREQUAL "IBTOOL-NOTFOUND")
		message(SEND_ERROR "ibtool can not be found and is needed to compile the .xib files. It should have been installed with the Apple developer tools. The default system paths were searched in addition to ${OSX_DEVELOPER_ROOT}/usr/bin")
	endif ()

	# Make sure the 'Resources' Directory is correctly created before we build
	#add_custom_command (TARGET ${RKT_EXE_NAME} PRE_BUILD COMMAND echo ${CMAKE_BINARY_DIR}/\${CONFIGURATION}/${RKT_EXE_NAME}.app/Contents/Resources/appnib.nib)

	# Compile the .xib files using the 'ibtool' program with the destination being the app package
	add_custom_command (TARGET ${RKT_EXE_NAME} POST_BUILD
						  COMMAND ${IBTOOL} --errors --warnings --notices --output-format human-readable-text
								--compile ${CMAKE_CURRENT_BINARY_DIR}/\${CONFIGURATION}/${RKT_EXE_NAME}.app/Contents/Resources/appnib.nib
								${CMAKE_CURRENT_SOURCE_DIR}/data/macosx/appnib.xib
						  COMMENT "Compiling appnib.xib")
endif ()

# move the bass dll close to the executable
if (MSVC)
	add_custom_command(TARGET ${RKT_EXE_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/external/bass/win32/bass.dll" $<TARGET_FILE_DIR:${RKT_EXE_NAME}>)
endif ()
