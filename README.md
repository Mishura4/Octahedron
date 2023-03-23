# Octahedron
Octahedron is a rewrite of the [Tesseract game engine](http://tesseract.gg/), itself built upon the [Cube 2 engine](http://sauerbraten.org/) engine.
It aims to make extensive use of modern C++20 features and design practices, to capture the customizability and overall feeling of the engine, while improving modulability and maintainability. One of the main goals of Octahedron is to build upon the in-game editor, allowing creating new types of entities, adding scripted events, etc. While its predecessors Tesseract and Cube2/Sauerbraten are both simultanouesly a game and an engine, Octahedron aims to purely be a game engine.

# License
#### Octahedron is currently licensed under [CC BY-NC-ND 4.0](https://creativecommons.org/licenses/by-nc-nd/4.0/).
Feel free to pull the code, build it, run it, contribute via pull requests, however you may not distribute your modifications to the engine in any other way.
My goal is to eventually change the license to another much more permissive, such as LGPL or ZLIB, when I judge the engine is complete enough to stand on its own.

# Penteract
Penteract is the game part of Tesseract, rewritten to use Octahedron as its engine. It is meant as a proof-of-concept, a skeleton for a game written with the Octahedron engine. **So far most of the code is taken directly from Tesseract,** as I slowly modernize the code and work on integrating Octahedron. There may be indentation mismatch in penteract source files, it is a work in progress.

Penteract is licensed under the [ZLIB license](https://zlib.net/zlib_license.html), just like Tesseract. __**Note that this only applies to files in `src/penteract`.**__
You will need a Tesseract installation's `media/` folder to be able to execute the code. If you're looking to use Visual Studio, putting the folder under `run/` is recommended as the debugger is set-up to use `run/` as working directory.
