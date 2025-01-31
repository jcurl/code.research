set(LIBRARY stdext)
set(SOURCES
    ../include/stdext/expected.h
)

# There is nothing to build. When there is, add all necessary instructions (see
# libubench/src/CMakeLists.txt).

# We don't want to format these files, to keep them similar to the original to
# make it easier to compare.
#
# clangformat_setup(${SOURCES})
