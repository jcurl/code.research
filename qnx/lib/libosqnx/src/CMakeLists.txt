set(LIBRARY osqnx)
set(SOURCES
    ../include/osqnx/pids.h pids.cpp
    ../include/osqnx/asinfo.h asinfo.cpp
)
add_library(${LIBRARY} STATIC ${SOURCES})
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_link_libraries(${LIBRARY} ubench)

if(CMAKE_SYSTEM_VERSION VERSION_LESS 8.0)
    # QNX 8.0 and later automatically include the std::filesystem extensions
    target_link_libraries(${LIBRARY} c++fs)
endif()

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()

target_clangformat(TARGET ${LIBRARY} DIR . DIR_RECURSE ../include/osqnx)
