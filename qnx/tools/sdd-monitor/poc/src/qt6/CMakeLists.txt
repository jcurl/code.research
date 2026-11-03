qt_add_executable(sdd_monitor_poc
    src/main.cpp
    src/qt_main_view.h
    src/qt_main_view.cpp
    src/computer_list_model.h
    src/computer_list_model.cpp
    include/sdd/qt/qt_computer.h)

target_include_directories(sdd_monitor_poc PRIVATE include)
target_link_libraries(sdd_monitor_poc PRIVATE sdd_presenter Qt6::Widgets)
