add_subdirectory(infra)

target_clangformat(TARGET sddinfra DIR_RECURSE infra/include DIR_RECURSE infra/src DIR_RECURSE infra/test)
