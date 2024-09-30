# C++ Dependency Analyzer


The **C++ Dependency Analyzer** is a C++ library designed to analyze file dependencies within a C++ project, detect strongly connected components (SCCs), and generate topological sorts for these components. It provides a clear visualization of complex file dependencies, making it easier to understand and optimize the structure of your project.

### Key Features:

1. **File Dependency Detection**: 
   - Analyzes files and their dependencies to build a directed dependency graph.
   - Files with the same stem (e.g., `A.cpp` and `A.h`) are treated as a single file, and their dependencies are considered together.

2. **Strongly Connected Component (SCC) Detection**: 
   - Identifies cycles in the dependency graph and groups files that depend on each other into SCCs.
   - Ensures that no cycles exist between components in the next step.

3. **Topological Sorting of SCCs**: 
   - Provides a topologically sorted order of SCCs, ensuring that dependencies are resolved in the correct sequence.

4. **Transitive Dependency Pruning**: 
   - Simplifies the dependency graph by removing transitive dependencies. 
   - For example, if `A -> B` and `A -> C`, but `B -> C`, then `A -> C` will be removed, as the transitive dependency through `B` already exists.

5. **Mermaid Graph Generation**: 
   - Generates a graph compatible with the Mermaid diagram format for visualizing the structure of file dependencies.

6. **Depth Relations**: 
   - Establishes the minimum depth of each component within the dependency tree, providing deeper insights into the overall structure.


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
- argument for path to be skipped (e.g. tests, examples, ...etc)
- argument for what to display
- argument for limited edges based on depth? depth range?
