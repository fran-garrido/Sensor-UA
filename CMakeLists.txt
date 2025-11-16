cmake_minimum_required(VERSION 3.20)
project(SensorOPC LANGUAGES CXX)

# === DEPENDENCIAS ===
add_subdirectory(deps/open62541pp)

# === LEE version.h AUTOMÁTICAMENTE (ÚNICO FUENTE DE LA VERDAD) ===
file(READ "${CMAKE_SOURCE_DIR}/resources/version.h" VERSION_HEADER)

# Extrae APP_VERSION
string(REGEX MATCH "#define APP_VERSION[ \t]+\"([^\"]+)\"" _ ${VERSION_HEADER})
set(APP_VERSION ${CMAKE_MATCH_1})

# Extrae APP_NAME
string(REGEX MATCH "#define APP_NAME[ \t]+\"([^\"]+)\"" _ ${VERSION_HEADER})
set(APP_NAME ${CMAKE_MATCH_1})

# Extrae APP_AUTHOR
string(REGEX MATCH "#define APP_AUTHOR[ \t]+\"([^\"]+)\"" _ ${VERSION_HEADER})
set(APP_AUTHOR ${CMAKE_MATCH_1})

# Extrae APP_COMPANY
string(REGEX MATCH "#define APP_COMPANY[ \t]+\"([^\"]+)\"" _ ${VERSION_HEADER})
set(APP_COMPANY ${CMAKE_MATCH_1})

# === EJECUTABLE ===
add_executable(SensorOPC 
    src/main.cpp
    resources/app.rc
    resources/version.h  # ← Incluido para main.cpp
)

target_link_libraries(SensorOPC PRIVATE open62541pp)

target_include_directories(SensorOPC PRIVATE ${CMAKE_SOURCE_DIR}/resources)

# === ESTÁTICO (MinGW) ===
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static -lpthread")
    target_link_options(SensorOPC PRIVATE -static)
endif()

# === NOMBRE FINAL DEL .EXE ===
set_target_properties(SensorOPC PROPERTIES
    OUTPUT_NAME "Sensor OPC"
    VERSION ${APP_VERSION}
)

# === RUTA DE SALIDA ===
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)

# === INSTALADOR NSIS (USA version.h AUTOMÁTICAMENTE) ===
install(TARGETS SensorOPC DESTINATION .)
install(FILES resources/opcua.ico DESTINATION .)

set(CPACK_GENERATOR "NSIS")
set(CPACK_PACKAGE_NAME "${APP_NAME}")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${APP_NAME}_v${APP_VERSION}")
set(CPACK_NSIS_DISPLAY_NAME "${APP_NAME} - ${APP_AUTHOR}")
set(CPACK_NSIS_PACKAGE_NAME "${APP_NAME}")
set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/resources/opcua.ico")
set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/resources/opcua.ico")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/apolo6995")
set(CPACK_NSIS_CONTACT "fjgarrido95@gmail.com")
set(CPACK_PACKAGE_VENDOR "CIP Virgen del Camino")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Servidor OPC UA de simulación de sensores")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "CIP VDC\\\\${APP_NAME}")

include(CPack)