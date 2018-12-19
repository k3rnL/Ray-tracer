cmake_minimum_required(VERSION 3.5)

include(GNUInstallDirs)

project(Raytracer VERSION 1.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories("source")
file(GLOB_RECURSE SOURCES source/*.cpp)# include/*.hpp)
file(GLOB_RECURSE INCLUDES source/*.hpp)
add_executable(Raytracer ${SOURCES})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})
#Find any version 2.X of SFML
#See the FindSFML.cmake file for additional details and instructions
find_package(FSE REQUIRED)
include_directories(${FSE_INCLUDE_DIR})
# target_link_libraries(Raytracer ${FSE_LIBRARY})

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
# target_link_libraries(Raytracer ${SDL2_LIBRARY})

find_package(assimp REQUIRED)
if (!ASSIMP_FOUND)
  message(ERROR "Cannot find assimp")
endif()
include_directories(${ASSIMP_INCLUDE_DIRS})
# target_link_libraries(Raytracer ${ASSIMP_LIBRARIES})
message("${ASSIMP_INCLUDE_DIRS}")

list(APPEND CMAKE_PREFIX_PATH ${GLEW_LOCATION})
find_package(GLEW REQUIRED)
if (GLEW_FOUND)
  include_directories(${GLEW_INCLUDE_DIR})
  # target_link_libraries(Raytracer ${GLEW_LIBRARIES})
endif()

find_package(GLM REQUIRED)
if (GLM_FOUND)
  include_directories(${GLM_INCLUDE_DIRS})
endif()
message("Glew : ${GLEW_LIBRARIES}")

find_package(OpenGL REQUIRED)
find_package(Bullet REQUIRED)
include_directories(${BULLET_INCLUDE_DIRS})

find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIRS})

target_link_libraries(Raytracer ${SDL2_LIBRARY} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${ASSIMP_LIBRARIES} ${BULLET_LIBRARIES} ${FREETYPE_LIBRARIES} ${FSE_LIBRARY} ${OpenCL_LIBRARIES})
