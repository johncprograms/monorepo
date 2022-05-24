# monorepo

This is a Git copy of the monorepo I use for various projects and experiments. The UX for Git is pretty terrible, hence this is just the network copy.

# projects

- [builder](builder.md) -- The program that builds the rest of these projects.
- [graph](src/main_graph.cpp) -- A terrible, experimental graph plotter, so I can write ad-hoc visualizations.
- [text_editor](text_editor.md) -- The fully-featured text editor that I wrote according to my tastes, which I use every day for all my programming.
- [grid](src/main_grid.cpp) -- An experimental csv viewer/editor that allows linking between elements.
- [compiler](compiler.md) -- The latest experimental compiler I wrote, for a toy statically-typed language.
- [rt](rt.md) -- Experimental rendering code, containing rasterization algorithms, ray-tracing algorithms, and more.
- [sudoku](src/main_su.cpp) -- Old, experimental sudoku app.

There's some other smaller projects not really worth mentioning, because they're so small as to be trivial.

# how things are laid out

The toplevel is a bit of a dumping ground for command-line tools to administrate source control, builds, debuggers, opening editors, etc. Hence all the .bat files.

The src/ directory contains all the monorepo source, which is where all the interesting stuff lives.

Right now the source files are all organized flat under src/ with various prefixes meaning various things:
- c_ for the old, experimental compiler code I wrote. See main_jc2.cpp for the newer experimental compiler.
- ds_ for various useful datastructures and their related algorithms.
- glw_ for the graphics layer / windowing code.
- math_ for various mathematical code.
- ui_ for various user interface datastructures and related code.
