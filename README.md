# JUDGEMENT OF HEAVENLY SILVERCROSS

## Build

Build: `make` or `make debug` makes a Linux build.
Building for Windows: `make OS=windows`
Building for web: `make PLATFORM=web`

When running the build, it also builds libraries in `lib/`. If you want to then build
to another platform, you need to clean the lib builds by running `make full-clean`.

## Coding Style & Naming Conventions

- Prefer small, single‑purpose functions.
- utils.h contains many useful utility functions.
- Function names should follow a `verb_noun()`-ish format
- No useless comments. Explain the why, not the what.
- Please please please please don't add superfluous comments, prefer readable code symbols.

## Security & Configuration Tips

- Do not load assets from absolute paths; the game assumes `resources/` relative to the repo root.
- Validate array bounds and pool capacities (bullets/items) when modifying limits.
- Use `assert()` liberally.
