set(PROJECT visualize)
set(SOURCES
	hex.cpp
	main.cpp
	)

set(CMAKE_MODULE_PATH "/usr/local/share/SFML/cmake/Modules")
find_package(SFML 2 COMPONENTS network graphics window system)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

target_compile_options(${PROJECT} PRIVATE
	-std=c++1z -g
	)

# unix
include_directories(${SFML_INCLUDE_DIR})
target_link_libraries(${PROJECT}
	sfml-graphics sfml-window sfml-system
	)
# mac
#include_directories(${SFML_INCLUDE_DIR})
#target_link_libraries(${PROJECT} ${SFML_LIBRARIES})

