cmake_minimum_required(VERSION 3.20)
project(WaveBox)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} -std=c++14 -Wno-deprecated")
# can likely delete many of the below frameworks...CoreAudio, AudioToolbox, etc.
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Metal -framework Foundation -framework QuartzCore -framework IOKit -framework IOSurface -framework Cocoa -framework CoreAudio -framework AudioToolbox -framework AudioUnit -framework CoreServices -framework Carbon -framework CoreFoundation -framework Security")

include_directories(include)
include_directories(include/vulkan)
include_directories(include/freetype)
link_directories(libs)
link_directories(libs/vulkan)

add_executable(WaveBox src/main.cpp 
	src/GraphicsHandler.cpp src/GraphicsHandler.h 
	src/Ocean.cpp src/Ocean.h
	src/Camera.cpp src/Camera.h
	src/Scene.cpp src/Scene.h
	src/Drawable.cpp src/Drawable.h
	src/Mesh.cpp src/Mesh.h
	src/UI.cpp src/UI.h
	src/UIHandler.cpp src/UIHandler.h
)

# using an outdated libpng.a because my up-to-date build for the headers we're using didn't provide me one lol
target_link_libraries(${PROJECT_NAME} vulkan.1.2.176 glfw3 z png freetype bz2 -m64)
