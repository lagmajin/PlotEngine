# External Library Candidates

This is a holding note for libraries that may improve PlotEngine as a high-end IDE-style novel editor.

## Near-Term Candidates

- `tree-sitter`
  - Use for incremental parsing of Markdown, screenplay-like syntax, custom novel markup, outline extraction, and structural navigation.
  - Good fit for editor services because it is fast enough for live document updates.

- `diff-match-patch` or `dtl`
  - Use for AI revision review, protected-range validation, and readable before/after changes.
  - Prefer if the next milestone deepens AI edit approval and rollback flows.

- `QtKeychain`
  - Use for cross-platform secure API key storage.
  - Current Windows DPAPI path is acceptable, but QtKeychain is cleaner if macOS/Linux support matters.

## Medium-Term Candidates

- `KSyntaxHighlighting`
  - Use if the editor needs robust syntax highlighting themes, Markdown-like grammars, or custom markup highlighting beyond the current lightweight highlighter.

- `libgit2`
  - Use for project history, snapshots, AI-edit checkpoints, and local version control without shelling out to Git.
  - Particularly useful once revisions become first-class project data.

## Docking Alternative

- `KDDockWidgets`
  - Alternative to QADS, not something to add immediately.
  - Worth evaluating only if QADS becomes limiting for multi-window IDE layouts, advanced docking persistence, or Qt version compatibility.

## Current Docking Decision

- Keep `qt-advanced-docking-system` as the active docking library for now.
- QADS is already integrated through vcpkg and now backs the main Explorer, Notes, Search, and Editor panes.
