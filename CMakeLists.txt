cmake_minimum_required(VERSION 3.11)

project(tittat VERSION 1.0)

set(CMAKE_C_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/runtime)
set(CONTENT_OUTPUT_DIR ${CMAKE_BINARY_DIR})
if (EMSCRIPTEN)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --shell-file ${CMAKE_SOURCE_DIR}/source/shell.html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_GLFW=3 -s ASSERTIONS=1 -s WASM=1 -s ASYNCIFY")
    set(CMAKE_EXECUTABLE_SUFFIX ".html") # This line is used to set your executable to build with the emscripten html template so that you can directly open it.
	set(CONTENT_OUTPUT_DIR ${CMAKE_BINARY_DIR})
endif ()


configure_file(source/tittat_config.h.in source/tittat_config.h)

add_subdirectory(upstream)

add_executable(tittat)
target_sources(tittat PRIVATE source/tittat.c source/misc_pasta.c)
target_include_directories(tittat PRIVATE source)
target_include_directories(tittat PRIVATE ${CMAKE_BINARY_DIR}/source)
target_link_libraries(tittat PRIVATE raylib)

#set(VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TARGET tittat PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(tittat PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

target_compile_options(tittat PRIVATE 
	$<$<CXX_COMPILER_ID:MSVC>:
		/W4 # Warning level 4
		/WX # Warnings as errors
		/wd4189 # local variable is initialized but not referenced
		/wd4101 # unreferenced local variable
		/wd4200 # nonstandard extension used: zero-sized array in struct/union
		/wd4100 # '': unreferenced formal parameter
	>
)

if (EMSCRIPTEN)
	set_target_properties(tittat PROPERTIES LINK_FLAGS "--preload-file ${CONTENT_OUTPUT_DIR}/content@content")
endif ()

add_custom_command(
	TARGET tittat PRE_LINK
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_CURRENT_SOURCE_DIR}/content
		${CONTENT_OUTPUT_DIR}/content
)