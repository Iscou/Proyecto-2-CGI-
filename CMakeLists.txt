cmake_minimum_required(VERSION 3.14)
project(PROY2_32111550)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

# Descargar GLM automaticamente
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Descargar ImGui automaticamente
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(imgui)

# Descargar TinyObjLoader automaticamente  
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)

# Archivos fuente de ImGui + Backends de GLFW y OpenGL3
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Buscar todas las subcarpetas de src
file(GLOB_RECURSE HEADER_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/src/*")
set(SRC_SUBDIRS "")
foreach(dir ${HEADER_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND SRC_SUBDIRS ${dir})
    endif()
endforeach()

# Rutas de encabezados (.h / .hpp)
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${SRC_SUBDIRS}
    ${glm_SOURCE_DIR}
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${tinyobjloader_SOURCE_DIR}
)

# Ruta de la carpeta de lib local 
link_directories(${CMAKE_SOURCE_DIR}/lib)

# Busca todos los .cpp y .c dentro de src/ y sus subcarpetas
file(GLOB_RECURSE PROJECT_SOURCES 
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.c"
)

# Crear el ejecutable
add_executable(proyecto2
    ${PROJECT_SOURCES}
    ${IMGUI_SOURCES}
)

# Enlazar librerias
target_link_libraries(proyecto2 glfw3 opengl32)

# Copiar glfw3.dll automaticamente al lado del .exe al compilar
#add_custom_command(TARGET proyecto2 POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_if_different
#    "${CMAKE_SOURCE_DIR}/lib/glfw3.dll"
#    $<TARGET_FILE_DIR:proyecto2>
#)

file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})