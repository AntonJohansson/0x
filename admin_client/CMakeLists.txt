set(PROJECT admin_client)
set(SOURCES
	main.cpp
	third_party/imgui-sfml/imconfig-SFML.h
	third_party/imgui-sfml/imgui-SFML.cpp
	third_party/imgui/imgui.cpp
	third_party/imgui/imgui_draw.cpp
	third_party/imgui/imgui_widgets.cpp
	)


project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

set(CMAKE_MODULE_PATH "/usr/local/share/SFML/cmake/Modules")
find_package(SFML 2 COMPONENTS network graphics window system)

find_library(library_location NAMES liblibrary.so HINTS ${CMAKE_BINARY_DIR}/library)
add_library(library0x SHARED IMPORTED)
set_target_properties(library0x PROPERTIES IMPORTED_LOCATION ${library_location})

target_include_directories(${PROJECT} PUBLIC 
	${CMAKE_BINARY_DIR}/library/
	third_party/imgui/
	third_party/imgui-sfml/
	)

target_compile_options(${PROJECT} PRIVATE
	-std=c++1z -g
	-DIMGUI_DISABLE_INCLUDE_IMCONFIG_H
	-include imconfig-SFML.h
	)

include_directories(${SFML_INCLUDE_DIR})

if(UNIX AND NOT APPLE)
	# unix
	target_link_libraries(${PROJECT}
		library0x sfml-graphics sfml-window sfml-system pthread GL
		)
elseif(APPLE)
	# mac
	target_link_libraries(${PROJECT} ${SFML_LIBRARIES} pthread)
endif()

