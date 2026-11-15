set(LIBRARY sddcore)
set(SOURCES
    ../include/sdd/computer.h
    ../include/sdd/computer_observer.h
    ../include/sdd/computer_service.h
    ../include/sdd/local_computer_service.h
    local_computer_service.cpp
    packet_parse_computer.cpp
    packet_parse_computer.h
)

find_package(Threads REQUIRED)

add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "sddcore")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(${LIBRARY} PUBLIC sddinfra libunet)
target_link_libraries(${LIBRARY} PRIVATE libsjson Threads::Threads)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()
