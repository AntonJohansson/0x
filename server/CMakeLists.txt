set(PROJECT server)
set(SOURCES
	game_refactor.cpp
	server.cpp
	main.cpp
	)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})

find_library(library_location NAMES liblibrary.so HINTS ${CMAKE_BINARY_DIR}/library)
add_library(library0x SHARED IMPORTED)
set_target_properties(library0x PROPERTIES IMPORTED_LOCATION ${library_location})

target_include_directories(${PROJECT} PUBLIC ${CMAKE_BINARY_DIR}/library/)


target_compile_options(${PROJECT} PRIVATE
	-std=c++17 -g
	)

target_link_libraries(${PROJECT}
	library0x
	pthread
	)
