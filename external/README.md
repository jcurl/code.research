# External

This contains 3rd party external libraries. One of the reasons to use submodules
here are:

- Exclude from the project `.clang-tidy` rules, so we don't get spurious errors
  for 3rd party code;
- Don't use `FetchContent` to download during the build - use the specific
  version given here;
- Share across multiple projects.
