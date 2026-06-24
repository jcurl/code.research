set(LIBRARY libsjson)
set(SOURCES
    ../include/sjson/json_writer.h
    json_writer.cpp
    json_writer_context.h
    json_writer_context.cpp
    json_writer_array.cpp
    json_writer_object.cpp
)

add_library(${LIBRARY} STATIC ${SOURCES})
set_target_properties(${LIBRARY} PROPERTIES OUTPUT_NAME "sjson")
target_compile_features(${LIBRARY} PRIVATE cxx_std_17)
target_include_directories(${LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if(IS_DEBUG)
    add_sanitizers(${LIBRARY})
endif()
