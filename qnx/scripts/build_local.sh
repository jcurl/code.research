#!/bin/bash

set -u

usage() {
  echo "Usage: $0 <clean|full|incremental> [target]" >&2
  exit 1
}

clean_workspace() {
  dir=$1
  if [ -d "$dir" ]; then
    cd "$dir" || exit 1
    rm -rf ./*
  fi
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  usage
fi

verb="${1}"
if [ "$verb" != "clean" ] && [ "$verb" != "full" ] && [ "$verb" != "incremental" ]; then
  usage
fi

if [ "$verb" = "clean" ] && [ "$#" -gt 1 ]; then
  usage
fi

target="${2:-}"

if [ -z "${VSCODE_WORKSPACE:-}" ]; then
  echo "Error: VSCODE_WORKSPACE is not set." >&2
  exit 1
fi

build_parent="$VSCODE_WORKSPACE/build"
if [ ! -d "$build_parent" ]; then
  echo "Error: parent directory does not exist: $build_parent" >&2
  exit 1
fi

build_dir="$VSCODE_WORKSPACE/build/local"
if [ ! -d "$build_dir" ]; then
  if ! mkdir "$build_dir"; then
    echo "Error: failed to create workspace directory: $build_dir" >&2
    exit 1
  fi
  verb="full"
elif [ ! -f "$build_dir/compile_commands.json" ]; then
  verb="full"
fi

if [ "$verb" = "clean" ]; then
  clean_workspace $build_dir
  exit 0
fi

if [ "$verb" = "full" ]; then
  clean_workspace $build_dir

  cmake -E make_directory "$build_dir"
  cmake -G Ninja -S "$VSCODE_WORKSPACE" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=DEBUG \
    -DCMAKE_RUN_CLANG_TIDY=OFF \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DSANITIZE_THREAD=OFF \
    -DCODE_COVERAGE=OFF
fi

cd "$build_dir" || exit 1
if [ -n "$target" ]; then
  cmake --build . --target "$target"
else
  cmake --build .
fi
