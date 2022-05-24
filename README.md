# monorepo

This is a Git copy of the monorepo I use for various projects and experiments. The UX for Git is pretty terrible, hence this is just the network copy.

# how things are laid out

The toplevel is a bit of a dumping ground for command-line tools to administrate source control, builds, debuggers, opening editors, etc. Hence all the .bat files.

The src/ directory contains all the monorepo source, which is where all the interesting stuff lives.

Right now the source files are all organized flat under src/ with various prefixes meaning various things:
- c_ for the old, experimental compiler code I wrote. See main_jc2.cpp for the newer experimental compiler.
- ds_ for various useful datastructures and their related algorithms.
- glw_ for the graphics layer / windowing code.
- math_ for various mathematical code.
- ui_ for various user interface datastructures and related code.

# projects

- [builder](builder.md) -- main_build.cpp -- The program that builds the rest of these projects.
- graph -- main_graph.cpp -- A terrible, experimental graph plotter, so I can write ad-hoc visualizations.
- [text_editor](text_editor.md) -- main_te.cpp -- The fully-featured text editor that I wrote according to my tastes, which I use every day for all my programming.
- [grid](grid.md) -- main_grid.cpp -- An experimental csv viewer/editor that allows linking between elements.
- [compiler](compiler.md) -- main_jc2.cpp -- The latest experimental compiler I wrote, for a toy statically-typed language.
- [rt](rt.md) -- main_rt.cpp -- Experimental rendering code, containing rasterization algorithms, ray-tracing algorithms, and more.
- sudoku -- main_su.cpp -- Old, experimental sudoku app.
- trailspace -- main_trailspace.cpp -- Old command-line program to remove trailing whitespace from text files. Not very useful anymore now that the text editor does that in one command.
