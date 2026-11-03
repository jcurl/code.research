add_library(sdd_presenter STATIC
    src/main_presenter.cpp
    include/sdd/computer.h
    include/sdd/i_main_view.h
    include/sdd/i_view_listener.h
    include/sdd/main_presenter.h)

target_include_directories(sdd_presenter PUBLIC include)
target_compile_features(sdd_presenter PUBLIC cxx_std_17)
