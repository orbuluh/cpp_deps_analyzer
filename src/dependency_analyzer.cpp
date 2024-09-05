#include "dependency_analyzer.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

const std::set<std::string> DependencyAnalyzer::empty_set_;

DependencyAnalyzer::DependencyAnalyzer(const std::vector<File>& files)
    : files_(files) {}

void DependencyAnalyzer::AnalyzeDependencies() {
  BuildFileDependencies();
  BuildClassDependencies();
  FindStronglyConnectedComponents();
}

void DependencyAnalyzer::FindStronglyConnectedComponents() {
  std::map<std::string, int> index;
  std::map<std::string, int> lowlink;
  std::map<std::string, bool> on_stack;
  std::vector<std::string> stack;
  int index_counter = 0;

  for (const auto& [node, _] : file_dependencies_) {
    if (index.find(node) == index.end()) {
      TarjanSCC(node, stack, index, lowlink, on_stack, index_counter);
    }
  }
}

void DependencyAnalyzer::TarjanSCC(const std::string& node,
                                   std::vector<std::string>& stack,
                                   std::map<std::string, int>& index,
                                   std::map<std::string, int>& lowlink,
                                   std::map<std::string, bool>& on_stack,
                                   int& index_counter) {
  index[node] = index_counter;
  lowlink[node] = index_counter;
  index_counter++;
  stack.push_back(node);
  on_stack[node] = true;

  for (const auto& neighbor : file_dependencies_[node]) {
    if (index.find(neighbor) == index.end()) {
      TarjanSCC(neighbor, stack, index, lowlink, on_stack, index_counter);
      lowlink[node] = std::min(lowlink[node], lowlink[neighbor]);
    } else if (on_stack[neighbor]) {
      lowlink[node] = std::min(lowlink[node], index[neighbor]);
    }
  }

  if (lowlink[node] == index[node]) {
    std::vector<std::string> component;
    std::string w;
    do {
      w = stack.back();
      stack.pop_back();
      on_stack[w] = false;
      component.push_back(w);
    } while (w != node);
    strongly_connected_components_.push_back(component);
  }
}

void DependencyAnalyzer::PrintDependencies() {
  std::cout << "File Dependencies:\n\n";
  for (const auto& [file, deps] : file_dependencies_) {
    std::cout << file << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }

  std::cout << "\n\nClass Dependencies:\n\n";
  for (const auto& [class_name, deps] : class_dependencies_) {
    std::cout << class_name << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }

  std::cout << "\n\nTopological Sort of Files:\n\n";
  auto sorted_files = TopologicalSort(file_dependencies_);
  for (const auto& file : sorted_files) {
    std::cout << file << "\n";
  }

  std::cout << "\n\nMermaid Graph Syntax:\n\n";
  std::cout << "```mermaid\n";
  std::cout << GenerateMermaidGraph();
  std::cout << "```\n";

  PrintMaxDepthOrCycles();
}

void DependencyAnalyzer::BuildFileDependencies() {
  for (const auto& file : files_) {
    for (const auto& header : file.included_headers) {
      // Find the full path of the header file
      // only care about the file that are under the user specified directory
      auto it = std::find_if(
          files_.begin(), files_.end(),
          [&header](const File& f) { return f.name.ends_with(header); });
      if (it != files_.end()) {
        file_dependencies_[file.name].insert(it->name);
      }
    }
  }
}

void DependencyAnalyzer::BuildClassDependencies() {
  for (const auto& file : files_) {
    for (const auto& class_name : file.defined_classes) {
      for (const auto& header : file.included_headers) {
        class_dependencies_[class_name].insert(header);
      }
    }
  }
}

std::vector<std::string> DependencyAnalyzer::TopologicalSort(
    const std::unordered_map<std::string, std::set<std::string>>& dependencies)
    const {
  std::vector<std::string> result;
  std::set<std::string> visited;

  for (const auto& [node, _] : dependencies) {
    if (visited.find(node) == visited.end()) {
      Dfs(node, dependencies, visited, result);
    }
  }

  std::reverse(result.begin(), result.end());
  return result;
}

std::vector<std::string> DependencyAnalyzer::GetTopologicallySortedFiles()
    const {
  return TopologicalSort(file_dependencies_);
}

void DependencyAnalyzer::Dfs(
    std::string_view node,
    const std::unordered_map<std::string, std::set<std::string>>& dependencies,
    std::set<std::string>& visited, std::vector<std::string>& result) const {
  visited.insert(std::string(node));

  if (dependencies.find(std::string(node)) != dependencies.end()) {
    for (const auto& neighbor : dependencies.at(std::string(node))) {
      if (visited.find(neighbor) == visited.end()) {
        Dfs(neighbor, dependencies, visited, result);
      }
    }
  }

  result.push_back(std::string(node));
}

std::optional<const std::set<std::string>*>
DependencyAnalyzer::GetFileDependenciesFor(std::string_view file) const {
  auto it = file_dependencies_.find(std::string(file));
  if (it != file_dependencies_.end()) {
    return &(it->second);
  }
  return std::nullopt;
}

std::optional<const std::set<std::string>*>
DependencyAnalyzer::GetClassDependenciesFor(std::string_view class_name) const {
  auto it = class_dependencies_.find(std::string(class_name));
  if (it != class_dependencies_.end()) {
    return &(it->second);
  }
  return std::nullopt;
}

// Add this new method implementation
std::string DependencyAnalyzer::GenerateMermaidGraph() const {
  std::stringstream mermaid;
  mermaid << "graph TD\n";

  // Create a map of files to their component index
  std::unordered_map<std::string, int> file_to_component;
  for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
    for (const auto& file : strongly_connected_components_[i]) {
      file_to_component[file] = i;
    }
  }

  // Generate subgraphs for each strongly connected component with more than one
  // element
  for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
    if (strongly_connected_components_[i].size() > 1) {
      mermaid << "    subgraph SCC" << i << "\n";
      for (const auto& file : strongly_connected_components_[i]) {
        mermaid << "        " << EscapeFileName(file) << "\n";
      }
      mermaid << "    end\n";
    } else {
      // For single-element SCCs, just output the node
      mermaid << "    " << EscapeFileName(strongly_connected_components_[i][0])
              << "\n";
    }
  }

  // Generate edges between components or individual nodes
  for (const auto& [file, deps] : file_dependencies_) {
    int from_component = file_to_component[file];
    std::string from_node =
        strongly_connected_components_[from_component].size() > 1
            ? "SCC" + std::to_string(from_component)
            : EscapeFileName(file);

    for (const auto& dep : deps) {
      int to_component = file_to_component[dep];
      std::string to_node =
          strongly_connected_components_[to_component].size() > 1
              ? "SCC" + std::to_string(to_component)
              : EscapeFileName(dep);

      if (from_node != to_node) {
        mermaid << "    " << from_node << " --> " << to_node << "\n";
      }
    }
  }

  return mermaid.str();
}

// Add this helper method to escape special characters in file names
std::string DependencyAnalyzer::EscapeFileName(
    std::string_view fileName) const {
  std::string escaped(fileName);
  std::replace(escaped.begin(), escaped.end(), '/', '_');
  std::replace(escaped.begin(), escaped.end(), '\\', '_');
  std::replace(escaped.begin(), escaped.end(), '.', '_');
  std::replace(escaped.begin(), escaped.end(), '-', '_');
  return escaped;
}

void DependencyAnalyzer::PrintMaxDepthOrCycles() const {
  if (HasCycles()) {
    std::cout << "Cycles detected in the dependency graph:\n";
    auto cycles = FindAllCycles();
    for (const auto& cycle : cycles) {
      std::cout << "  ";
      for (const auto& node : cycle) {
        std::cout << node << " -> ";
      }
      std::cout << cycle.front() << "\n";
    }
  } else {
    std::cout << "Max Graph Depth: " << GetMaxGraphDepth() << "\n";
  }
}

bool DependencyAnalyzer::HasCycles() const { return !FindAllCycles().empty(); }

std::vector<std::vector<std::string>> DependencyAnalyzer::FindAllCycles()
    const {
  std::vector<std::vector<std::string>> cycles;
  std::set<std::string> visited;
  std::vector<std::string> path;

  for (const auto& [node, _] : file_dependencies_) {
    if (visited.find(node) == visited.end()) {
      DfsForCycles(node, path, visited, cycles);
    }
  }

  return cycles;
}

void DependencyAnalyzer::DfsForCycles(
    const std::string& node, std::vector<std::string>& path,
    std::set<std::string>& visited,
    std::vector<std::vector<std::string>>& cycles) const {
  visited.insert(node);
  path.push_back(node);

  auto it = file_dependencies_.find(node);
  if (it != file_dependencies_.end()) {
    for (const auto& neighbor : it->second) {
      if (std::find(path.begin(), path.end(), neighbor) != path.end()) {
        // Cycle detected
        auto cycle_start = std::find(path.begin(), path.end(), neighbor);
        cycles.push_back(std::vector<std::string>(cycle_start, path.end()));
      } else if (visited.find(neighbor) == visited.end()) {
        DfsForCycles(neighbor, path, visited, cycles);
      }
    }
  }

  path.pop_back();
}

// Modify the existing GetMaxGraphDepth method
int DependencyAnalyzer::GetMaxGraphDepth() const {
  if (HasCycles()) {
    return -1;  // Indicate that cycles are present
  }

  std::unordered_map<std::string, int> depth_cache;
  int max_depth = 0;

  for (const auto& [file, _] : file_dependencies_) {
    max_depth = std::max(max_depth, CalculateMaxDepth(file, depth_cache));
  }

  return max_depth;
}

int DependencyAnalyzer::CalculateMaxDepth(
    const std::string& node,
    std::unordered_map<std::string, int>& depth_cache) const {
  // Check if the depth is already calculated
  auto it = depth_cache.find(node);
  if (it != depth_cache.end()) {
    return it->second;
  }

  int max_child_depth = 0;
  auto deps_it = file_dependencies_.find(node);
  if (deps_it != file_dependencies_.end()) {
    for (const auto& child : deps_it->second) {
      max_child_depth =
          std::max(max_child_depth, CalculateMaxDepth(child, depth_cache));
    }
  }

  // Use insert() instead of operator[] to avoid potential issues
  depth_cache.insert({node, 1 + max_child_depth});
  return 1 + max_child_depth;
}
