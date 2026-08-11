# JUDGEMENT OF HEAVENLY SILVERCROSS

## Build

Build: `make` or `make debug` makes a Linux build.
Building for Windows: `make OS=windows`
Building for web: `make PLATFORM=web`

When running the build, it also builds libraries in `lib/`. If you want to then build
to another platform, you need to clean the lib builds by running `make full-clean`.
