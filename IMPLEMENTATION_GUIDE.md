# PlotEngine Implementation Guide

## Goals

- Keep the project self-contained and reproducible through `vcpkg` manifest mode.
- Require C++20/23 modules for project domain code.
- Keep Qt UI code working first, then migrate internals gradually.
- Require `import std;` in project module units.

## Dependency Policy

- Qt6 is a hard dependency.
- `qt-advanced-docking-system` is the preferred dock framework for the IDE shell.
- Dependency versions should be pinned through `vcpkg.json` and `vcpkg-configuration.json`.
- Do not rely on a machine-wide Qt install once the manifest-based flow is in place.
- The CMake preset assumes `VCPKG_ROOT` points at a local vcpkg checkout.
- The primary configure preset uses `Ninja` and enables the standard library module path.
- CMake 3.30+ is required for `CMAKE_CXX_MODULE_STD`.

## Module Policy

- New domain code must be written as `.ixx` or `.cppm`.
- Prefer module interfaces for stable boundaries and implementation units for internals.
- Keep Qt-heavy classes in headers until the module boundary is proven safe.
- Move model, serialization, formatting, and project logic into modules before the UI shell.
- Current module boundaries: `PlotEngine.Core.TextUtils`, `PlotEngine.Core.NovelProject`, `PlotEngine.Core.ProjectManager`.
- Treat legacy header/source pairs for domain logic as migration targets, not the desired steady state.
- Do not migrate an `ixx`/`cppm` area back to classic headers/sources unless the user explicitly requests that rollback.

## `import std`

- `import std;` is mandatory in project-owned module units.
- Do not replace `import std;` with piecemeal standard headers in module units unless the project policy changes.
- Avoid mixing `import std;` into legacy translation units unless it reduces friction.
- Prefer standard library facilities over custom utility code when modules make the dependency graph simpler.

## Verdigris / `W_OBJECT`

- Vendored Verdigris headers live under `third_party/verdigris/`.
- Use `W_OBJECT`, `W_SIGNAL`, `W_SLOT`, and `W_OBJECT_IMPL` when a class needs Qt-style signals/slots without `moc`.
- Keep the vendored headers unchanged unless a deliberate upstream sync is planned.
- Treat Verdigris as infrastructure for module-friendly Qt code, not as a reason to avoid cleanup.

## Migration Order

1. Establish `vcpkg` manifest mode and build presets.
2. Switch the project to Qt6 + ADS from `vcpkg`.
3. Introduce one small module for non-Qt domain logic.
4. Convert serialization and project model code to modules.
5. Move UI shell pieces last, after the boundaries are stable.

## Practical Rules

- Do not rewrite the whole codebase into modules in one pass.
- Keep the first module small, testable, and low-risk.
- Preserve existing behavior before changing architecture.
- Prefer incremental refactors that can be compiled after each step.
- When adding or refactoring domain code, prefer changing the build to support modules rather than falling back to non-module structure.
