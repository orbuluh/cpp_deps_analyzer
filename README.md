# C++ Deps Analyzer

Simple tools to dump topological orders of files for cpp codebase

## Compile

1. Clone the repository:
   ```
   git clone https://github.com/yourusername/cpp-codebase-analyzer.git
   ```

2. Navigate to the project directory:
   ```
   cd cpp-codebase-analyzer
   ```

3. Build the project:
   ```
   mkdir build && cd build
   cmake ..
   make
   ```


4. For a debug build, use the following commands instead:
   ```
   mkdir debug_build && cd debug_build
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   make
   ```

   This will create a debug build with additional debugging information and without optimizations.


## TODOs

- abstract building the dependency graph - so it could be applied to different languages?
