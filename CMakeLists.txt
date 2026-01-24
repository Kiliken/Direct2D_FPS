cmake_minimum_required(VERSION 3.15)
project(AppProject)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add Defines
#add_compile_definitions(RELEASE)

# Executable Files
add_executable(App

	# Main
    src/main.cpp
	src/raycastTest.cpp
	src/EnemyManager.cpp
	src/Enemy.cpp
	src/Player.h
	src/Pathfinding.cpp


	# ImGui core
	dep/include/imgui/imgui.cpp
	dep/include/imgui/imgui_draw.cpp
	dep/include/imgui/imgui_tables.cpp
	dep/include/imgui/imgui_widgets.cpp
	dep/include/imgui/imgui_demo.cpp
	
	# ImGui backends
	dep/include/imgui/imgui_impl_sdl3.cpp
    dep/include/imgui/imgui_impl_dx11.cpp
)

# Include Directories
target_include_directories(App PRIVATE 
    src 
    dep/include
)

# Link Directories
target_link_directories(App PRIVATE 
    src 
    dep/lib
)

# Link Libraries
target_link_libraries(App PRIVATE 
    opengl32
	user32
	shell32
	gdi32
	kernel32
	advapi32
	ole32
	oleaut32
	uuid
	odbc32
	odbccp32
	SDL3
	d3d11
	D2d1
)