# Project working rules

These preferences apply to all work in this repository.

1. **Explain the plan first.** Before starting work or calling tools, briefly tell
   the user what you intend to do. Explain material changes to the plan as they arise.
2. **Test only the changes.** Run focused tests for changed behavior and directly
   affected integrations. Do not run the entire test suite or unrelated tests by
   default. Expand testing only when a failure or a concrete dependency warrants it,
   and explain why first. Documentation-only changes need documentation checks,
   not application test runs.
3. **Work one step at a time and wait between steps.** Do not execute an entire batch
   at once. Break each batch down into clear, individual steps so the user can follow along.
   Verify the current step first, report the results, state what the next step is, and stop.
   Wait for the user's explicit instruction before starting the next step.
4. **Full access within QuantumChaos.** The assistant has unconditional permission
   to write, read, and edit any file within this directory
   `/home/iusearchbtw/ThisPC/C++ projects/QuantumChaos` and all its subfolders.
   Do not ask the user for permission in chat to create, read, or edit files in this repository.
5. **Preserve existing features & prevent regressions.** Never break, degrade, or
   accidentally remove existing functionality when fixing bugs or implementing new
   features. Verify that existing workflows, interactions, and features remain intact.
6. **Strictly follow the 3-tier architecture & nomenclature.** Maintain the project's
   architectural separation and naming conventions:
   - **Backend:** Pure C++20 logic and domain models (no UI/Qt dependencies). Built as the
     `notesengine` static library, which does not link Qt, so a Qt include fails to compile.
   - **Frontend:** Pure QML UI presentation and view logic (no business logic, no maths).
   - **Wrapper:** Qt C++ bridging/adapter layer exposing backend models and actions to QML.
     It also moves bytes (sockets, platform calls) but makes no decisions about them.
   - **Naming:** one folder per component, same name in every layer:
     `backend/abc/abc.h` + `abc.cpp`, `wrapper/abc/abcwrap.h` + `abcwrap.cpp`,
     `frontend/abc/Abc.qml`. QML file names start with a capital letter and are unique
     across all subfolders.
7. **Theme and icons in one place.** `frontend/theme/Theme.qml` holds every colour and size.
   `frontend/icons/Icons.qml` holds every icon.
8. **Few dependencies, each behind your own interface.** The C++ standard library, Qt
   (wrapper and frontend only), nlohmann/json and Monocypher. Monocypher is only reached
   through `backend/crypto/`. Both are vendored in `dependencies/` so Android and Windows
   builds need no system packages. A new dependency needs a very good reason.
9. **Placeholders are fine.** If a piece of plumbing blocks you, use a simple placeholder and carry on.
10. **Keep the map current.** Update `EducationPurpose/architecture.txt` when files are added or removed.
11. **Tests never touch real data.** Every test and test app run uses a throwaway data and
    settings folder, never the user's real notes or settings.
12. **Three targets.** Linux (tarball), Windows (installer) and Android (APK). No
    machine-specific compiler flags such as `-march=x86-64-v3`, because they break ARM builds.
13. **No other contributors.** The project owner is the only author. No `Co-Authored-By`
    trailers, no "generated with" lines, and no credit to any tool, assistant or company in
    commit messages, pull requests, README files, code comments or any other file.
