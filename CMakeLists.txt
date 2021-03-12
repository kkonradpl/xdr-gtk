cmake_minimum_required(VERSION 3.6)
project(xdr-gtk)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK REQUIRED gtk+-2.0)
include_directories(${GTK_INCLUDE_DIRS})
link_directories(${GTK_LIBRARY_DIRS})
add_definitions(${GTK_CFLAGS_OTHER})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -Wno-deprecated-declarations")

add_subdirectory(src)

if(NOT MINGW)
    install(TARGETS xdr-gtk DESTINATION bin)
    install(FILES xdr-gtk.desktop DESTINATION share/applications)
    install(DIRECTORY icons/ DESTINATION share/icons/hicolor
            PATTERN "INFO" EXCLUDE
            PATTERN "ico" EXCLUDE)
    install(CODE "execute_process(COMMAND gtk-update-icon-cache /usr/share/icons/hicolor)")
endif()
