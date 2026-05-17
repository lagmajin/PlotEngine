"""
PlotEngine sample script.

The embedded runner passes `project` as a JSON-like dict.
Mutate it in place and the C++ project model will be updated after execution.
"""

project["name"] = project.get("name", "").strip() or "Untitled"
project["author"] = project.get("author", "").strip()

for chapter in project.get("chapters", []):
    chapter["title"] = chapter.get("title", "").strip()
    chapter["summary"] = chapter.get("summary", "").strip()
    for episode in chapter.get("episodes", []):
        episode["title"] = episode.get("title", "").strip()
        episode["summary"] = episode.get("summary", "").strip()
