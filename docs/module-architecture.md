# Module Architecture

PlotEngine is moving toward C++20/23 modules while keeping Qt widget classes stable under Verdigris.

## Current Module Boundaries

- `PlotEngine.App.Application`
  - Owns `QApplication` setup, application metadata, app icon loading, main window creation, and the event loop.
  - `src/main.cpp` stays as the minimal OS entry point.

- `PlotEngine.App.Metadata`
  - Owns application name, organization name, version, window title, and `QSettings` construction.
  - Avoids scattering `"PlotEngine"` settings identifiers across UI and AI code.

- `PlotEngine.UI.MainWindow`
  - Exports the main IDE shell.
  - Uses Verdigris instead of Qt `moc`.

- `PlotEngine.UI.DockPane`
  - Exports shared QADS pane descriptors such as `DockPaneSpec` and `DockPlacement`.

- `PlotEngine.UI.IconFactory`
  - Exports generated menu/toolbar icon creation.
  - Avoids file-based icon assets for action icons.

- `PlotEngine.Core.*`
  - Owns project model, project management, revision management, text utilities, and export utilities.

- `PlotEngine.AI.*`
  - Owns context building, revision prompts, AI data actions, and prompt implementation helpers.

- `PlotEngine.AI.SecretStore`
  - Owns AI API key persistence.
  - Uses Windows DPAPI on Windows and the shared app settings identity from `PlotEngine.App.Metadata`.

## Migration Rules

- Keep `import std;` in module units.
- Keep Qt `Q_OBJECT` out of the codebase; use Verdigris `W_OBJECT`.
- Prefer `.ixx`/`.cppm` for stable domain and UI support contracts.
- Keep high-churn QWidget implementations as regular `.cpp` until their exported surface is stable.
- Avoid importing C++ modules inside the global module fragment; put imports after the module declaration or in normal translation units.

## Next Good Candidates

- Move editor-neutral text helpers out of widget code and into modules.
- Split `MainWindow` commands into smaller modules once command state is less coupled to widgets.
- Consider module wrappers for AI provider request/response data after the provider interfaces settle.
