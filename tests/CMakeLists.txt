set(PROJECT tests)
set(SOURCES
		hex_tests.cpp
		memory_tests.cpp
		hex_map_tests.cpp
		crc_tests.cpp
		encoding_tests.cpp
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

include_directories(${CMAKE_SOURCE_DIR})

target_link_libraries(${PROJECT}
	library0x
	)
