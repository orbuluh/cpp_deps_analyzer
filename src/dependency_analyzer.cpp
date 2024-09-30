#include "dependency_analyzer.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

DependencyAnalyzer::DependencyAnalyzer(const std::vector<File>& files)
    : files_(files), max_depth_{0} {
  BuildFileDependencies();
  BuildSCC();
  BuildSCCDependencies();
  PruneTransitiveDependencies();
  TopologicalSortSCCDependencies();
  BuildMinDepthRelation();  // for better ordering of the output
}

void DependencyAnalyzer::BuildFileDependencies() {
  // with this, src_path/x.cpp and include_path/x.h will be considered
  // as the same file component.
  auto get_file_stem = [](const std::string& path) {
    return std::filesystem::path(path).stem().string();
  };
  for (const auto& file : files_) {
    for (const auto& header : file.included_headers) {
      // When dealing with included files, only check files that are under the
      // user specified directory. (e.g. those included by the files input)
      auto it = std::find_if(
          files_.begin(), files_.end(),
          [&header](const File& f) { return f.name.ends_with(header); });
      if (it != files_.end()) {
        file_deps_[get_file_stem(file.name)].insert(get_file_stem(it->name));
      } else {
        std::cout << "Skip included file: " << header
                  << " as it's not under user specified directory."
                  << std::endl;
      }
    }
  }
}

void DependencyAnalyzer::BuildSCC() {
  std::map<std::string, int> index;
  std::map<std::string, int> lowlink;
  std::map<std::string, bool> on_stack;
  std::vector<std::string> stack;
  int index_counter = 0;

  for (const auto& [node, _] : file_deps_) {
    if (index.find(node) == index.end()) {
      TarjanSCC(node, stack, index, lowlink, on_stack, index_counter);
    }
  }
  BuildSCCNames();
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

  for (const auto& neighbor : file_deps_[node]) {
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

void DependencyAnalyzer::BuildSCCNames() {
  assert(!strongly_connected_components_.empty());
  scc_name_.clear();  // Clear any existing values
  scc_name_.reserve(
      strongly_connected_components_.size());  // Reserve space for efficiency

  for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
    std::string concatenated_name;
    for (const auto& file : strongly_connected_components_[i]) {
      file_to_component_[file] = i;
      concatenated_name += file;
      concatenated_name += "|";  // Optional separator between file names
    }
    // Remove the trailing separator
    if (!concatenated_name.empty()) {
      concatenated_name.pop_back();
    }
    scc_name_.push_back(concatenated_name);  // Add to scc_name_
  }
}

void DependencyAnalyzer::PrintStronglyConnectedComponents() const {
  assert(!scc_name_.empty());
  std::cout << "\nStrongly Connected Components:\n\n";
  for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
    std::cout << "SCC[" << i << "](" << scc_name_[i] << "): ";
    for (const auto& file : strongly_connected_components_[i]) {
      std::cout << file << " ";
    }
    std::cout << std::endl;
  }
}

void DependencyAnalyzer::BuildSCCDependencies() {
  for (const auto& [file, dependencies] : file_deps_) {
    SccIdx component_idx = file_to_component_[file];  // Component of the file

    for (const auto& dep : dependencies) {
      SccIdx dep_component_idx =
          file_to_component_[dep];  // Component of the dependency

      // Add a dependency between different components
      if (component_idx != dep_component_idx) {
        component_deps_[component_idx].insert(dep_component_idx);
      }
    }
  }
}

void DependencyAnalyzer::PruneTransitiveDependencies() {
  // just copy the whole map
  simplified_component_deps_ = component_deps_;

  for (auto& [node, dependencies] : simplified_component_deps_) {
    std::unordered_set<SccIdx> transitive_dependencies;

    // Collect all transitive dependencies for the current node
    for (const auto& direct_dep : dependencies) {
      CollectTransitiveDependencies(direct_dep, transitive_dependencies);
    }

    // Remove direct dependencies that are also reachable transitively
    for (auto it = dependencies.begin(); it != dependencies.end();) {
      if (transitive_dependencies.find(*it) != transitive_dependencies.end()) {
        it = dependencies.erase(it);  // Remove transitive dependency
      } else {
        ++it;
      }
    }
  }
}

void DependencyAnalyzer::CollectTransitiveDependencies(
    SccIdx component_idx, std::unordered_set<SccIdx>& reachable_indices) {
  // If the component has no dependencies, return
  if (component_deps_.find(component_idx) == component_deps_.end()) {
    return;
  }

  // Recursively visit each neighbor (dependency)
  for (auto neighbor : component_deps_[component_idx]) {
    if (reachable_indices.find(neighbor) == reachable_indices.end()) {
      reachable_indices.insert(neighbor);
      CollectTransitiveDependencies(neighbor, reachable_indices);
    }
  }
}

void DependencyAnalyzer::TopologicalSortSCCDependencies() {
  if (!topoplogical_sorted_sccs_.empty()) {
    return;
  }

  // If no dependencies exist, just add all components to the sorted list
  if (simplified_component_deps_.empty()) {
    for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
      topoplogical_sorted_sccs_.push_back(i);  // Add each component index
    }
    return;
  }

  std::set<SccIdx> visited;
  // no need to only traverse from node without deps
  // as Dfs will ensure those nodes are visited first
  for (const auto& [node, _] : simplified_component_deps_) {
    if (visited.find(node) == visited.end()) {
      Dfs(node, visited);
    }
  }

  std::reverse(topoplogical_sorted_sccs_.begin(),
               topoplogical_sorted_sccs_.end());
}

void DependencyAnalyzer::Dfs(SccIdx component_idx, std::set<SccIdx>& visited) {
  visited.insert(component_idx);

  if (simplified_component_deps_.find(component_idx) !=
      simplified_component_deps_.end()) {
    for (const auto& neighbor : simplified_component_deps_.at(component_idx)) {
      if (visited.find(neighbor) == visited.end()) {
        Dfs(neighbor, visited);
      }
    }
  }

  topoplogical_sorted_sccs_.emplace_back(component_idx);
}

void DependencyAnalyzer::BuildMinDepthRelation() {
  //  the minimum depth of each node in a graph, where the depth of a node is
  //  defined as the longest path from that node to any node with no
  //  dependencies (a leaf node).
  assert(!topoplogical_sorted_sccs_.empty());

  for (auto it = topoplogical_sorted_sccs_.rbegin();
       it != topoplogical_sorted_sccs_.rend(); ++it) {
    const auto& component_idx = *it;
    int max_dependency_depth = 0;

    if (simplified_component_deps_.find(component_idx) !=
        simplified_component_deps_.end()) {
      for (const auto& neighbor :
           simplified_component_deps_.at(component_idx)) {
        max_dependency_depth =
            std::max(max_dependency_depth, depth_map_[neighbor] + 1);
      }
    }

    depth_map_[component_idx] = max_dependency_depth;
    depth_to_component_idx_map_[max_dependency_depth].push_back(component_idx);
    max_depth_ = std::max(max_depth_, max_dependency_depth);
  }
}

// Add this new method implementation
std::string DependencyAnalyzer::GenerateMermaidGraph() {
  std::stringstream mermaid;
  mermaid << "graph LR\n";

  // Generate subgraphs for each strongly connected component with more than
  // one element

  for (size_t i = 0; i < strongly_connected_components_.size(); ++i) {
    if (strongly_connected_components_[i].size() > 1) {
      mermaid << "    subgraph SCC_" << i << "\n";
      for (const auto& file : strongly_connected_components_[i]) {
        mermaid << "        " << file << "\n";
      }
      mermaid << "    end\n";
    } else {
      // For single-element SCCs, just output the node
      mermaid << "    " << scc_name_[i] << "\n";
    }
  }

  auto get_name = [&](SccIdx component_idx) {
    if (strongly_connected_components_[component_idx].size() > 1) {
      return "SCC_" + std::to_string(component_idx);
    } else {
      return scc_name_[component_idx];
    }
  };

  // Generate edges between components or individual nodes
  for (const auto& [from_component, deps] : simplified_component_deps_) {
    for (const auto& to_component : deps) {
      if (from_component != to_component) {
        mermaid << "    " << get_name(from_component) << " --> "
                << get_name(to_component) << "\n";
      }
    }
  }

  return mermaid.str();
}

void DependencyAnalyzer::PrintDependencies() {
  std::cout << "File Dependencies:\n\n";
  for (const auto& [file, deps] : file_deps_) {
    std::cout << file << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }

  PrintStronglyConnectedComponents();

  std::cout << "\n\nTopological Sort of Files (less deps on top):\n\n";

  for (size_t depth = 0; depth <= max_depth_; ++depth) {
    for (const auto& component_idx : depth_to_component_idx_map_[depth]) {
      std::cout << "[" << depth << "]: " << scc_name_[component_idx] << "\n";
    }
  }

  std::cout << "\n\nMermaid Graph Syntax:\n\n";
  std::cout << "```mermaid\n";
  std::cout << GenerateMermaidGraph();
  std::cout << "```\n";

  std::cout << "Max Graph Depth: " << max_depth_ + 1 << "\n";
}
