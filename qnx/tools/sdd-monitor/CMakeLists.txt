add_subdirectory(infra)
add_subdirectory(core)
add_subdirectory(cli)

target_clangformat(TARGET sddinfra DIR_RECURSE infra/include DIR_RECURSE infra/src DIR_RECURSE infra/test)
target_clangformat(TARGET sddcore DIR_RECURSE core/include DIR_RECURSE core/src DIR_RECURSE core/test)
