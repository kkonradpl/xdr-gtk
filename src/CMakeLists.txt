cmake_minimum_required(VERSION 3.6)

set(SOURCE_FILES
        conf.c
        conf.h
        conf-defaults.h
        log.c
        log.h
        main.c
        pattern.c
        pattern.h
        rds-utils.c
        rds-utils.h
        rdsspy.c
        rdsspy.h
        scan.c
        scan.h
        scheduler.c
        scheduler.h
        settings.c
        settings.h
        stationlist.c
        stationlist.h
        tuner.c
        tuner.h
        tuner-callbacks.c
        tuner-callbacks.h
        tuner-conn.c
        tuner-conn.h
        tuner-filters.c
        tuner-filters.h
        tuner-scan.c
        tuner-scan.h
        ui.c
        ui.h
        ui-audio.c
        ui-audio.h
        ui-connect.c
        ui-connect.h
        ui-input.c
        ui-input.h
        ui-signal.c
        ui-signal.h
        ui-tuner-set.c
        ui-tuner-set.h
        ui-tuner-update.c
        ui-tuner-update.h
        version.h)

set(SOURCE_FILES_MINGW
        win32.c
        win32.h
        icon.rc)

set(LIBRARIES
        ${GTK_LIBRARIES}
        m)

set(LIBRARIES_MINGW
        ws2_32)

if(MINGW)
    IF(NOT (CMAKE_BUILD_TYPE MATCHES Debug))
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mwindows")
    ENDIF()
    set(CMAKE_RC_COMPILER_INIT windres)
    ENABLE_LANGUAGE(RC)
    SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -i <SOURCE> -o <OBJECT>")
    add_executable(xdr-gtk ${SOURCE_FILES} ${SOURCE_FILES_MINGW})
    target_link_libraries(xdr-gtk ${LIBRARIES} ${LIBRARIES_MINGW})
ELSE()
    add_executable(xdr-gtk ${SOURCE_FILES})
    target_link_libraries(xdr-gtk ${LIBRARIES})
ENDIF()
