set(LIBRARY libosqnx)
set(SOURCES
    ../include/osqnx/asinfo.h asinfo.cpp
    ../include/osqnx/pids.h pids.cpp
    ../include/osqnx/qnxver.h
    ../include/osqnx/resmgr/device.h resmgr/device.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "osqnx")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
# Header files also use libubench includes, so this must be public.
target_link_libraries(${LIBRARY} PUBLIC libubench libstdext)

target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
    # QNX 8.0 and later automatically include the std::filesystem extensions
    target_link_libraries(${LIBRARY} PRIVATE c++fs)
else()
    target_link_libraries(${LIBRARY} PRIVATE secpol)
endif()

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()
