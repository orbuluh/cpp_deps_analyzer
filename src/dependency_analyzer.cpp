#include "dependency_analyzer.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

std::string GetFileNameFromPath(const std::string& path) {
  std::regex file_regex(R"([^/]+$)");
  std::smatch match;

  if (std::regex_search(path, match, file_regex)) {
    return match.str(0);  // Return the matched file name
  }

  return "";  // Return an empty string if no match is found
}

StrDepMap BuildFileDependencies(const std::vector<File>& files) {
  // with this, src_path/x.cpp and include_path/x.h will be considered
  // as the same file component.
  auto get_file_stem = [](const std::string& path) {
    return std::filesystem::path(path).stem().string();
  };

  StrDepMap file_deps;
  for (const auto& file : files) {
    for (const auto& header_path : file.included_headers) {
      // When dealing with included files, only check files that are under the
      // user specified directory. (e.g. those included by the files input)
      const auto header = GetFileNameFromPath(header_path);
      auto it = std::find_if(
          files.begin(), files.end(),
          [&header](const File& f) { return f.name.ends_with(header); });
      if (it != files.end()) {
        const auto src_stem = get_file_stem(file.name);
        const auto tgt_stem = get_file_stem(it->name);
        if (file_deps.find(src_stem) == file_deps.end()) {
          file_deps[src_stem] = {};
        }
        if (file_deps.find(tgt_stem) == file_deps.end()) {
          file_deps[tgt_stem] = {};
        }
        if (src_stem != tgt_stem) {
          file_deps[src_stem].insert(tgt_stem);
        }
      } else {
        std::cerr << "Skip included file: " << header << " for " << file.name
                  << " as it's not under user specified directory."
                  << std::endl;
      }
    }
  }
  return file_deps;
}

// -----------------------------------------------------------------------------

SCCBuilder::SCCBuilder(const StrDepMap& file_deps) : file_deps_(file_deps) {
  BuildSCC();
  BuildSCCNames();  // Build SCC names once the SCCs are detected
  BuildSCCDependencies();
}

void SCCBuilder::BuildSCC() {
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
}

void SCCBuilder::TarjanSCC(const std::string& node,
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

  for (const auto& neighbor : file_deps_.at(node)) {
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
    scc_components_.emplace_back("", component);  // Add SCC without name yet
  }
}

void SCCBuilder::BuildSCCNames() {
  assert(!scc_components_.empty());

  for (size_t i = 0; i < scc_components_.size(); ++i) {
    std::string concatenated_name;

    // For each SCC component, concatenate the member names
    for (const auto& file : scc_components_[i].members) {
      file_to_component_[file] = i;     // Map file to its component index
      concatenated_name += file + "|";  // Append file name with separator
    }

    // Remove trailing '|'
    if (!concatenated_name.empty()) {
      concatenated_name.pop_back();
    }

    // Assign concatenated name to the SCC component
    scc_components_[i].name = concatenated_name;
  }
}

std::optional<SccIdx> SCCBuilder::GetComponentIndex(
    const std::string& file) const {
  auto it = file_to_component_.find(file);
  if (it != file_to_component_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::string SCCBuilder::ToDescription() const {
  assert(!scc_components_.empty());
  std::ostringstream oss;
  oss << "\nStrongly Connected Components:\n\n";
  for (size_t i = 0; i < scc_components_.size(); ++i) {
    oss << "SCC[" << i << "](" << scc_components_[i].name << "): ";
    for (const auto& file : scc_components_[i].members) {
      oss << file << " ";
    }
    oss << std::endl;
  }
  return oss.str();
}

void SCCBuilder::BuildSCCDependencies() {
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

// -----------------------------------------------------------------------------

DependencyAnalyzer::DependencyAnalyzer(const std::vector<File>& files)
    : max_depth_{0},
      file_deps_{BuildFileDependencies(files)},
      scc_{file_deps_},
      components_vec_{scc_.GetSCCComponents()},
      simplified_component_deps_{scc_.GetSCCDeps()} {
  PruneTransitiveDependencies();
  TopologicalSortSCCDependencies();
  BuildMinDepthRelation();  // for better ordering of the output
}

void DependencyAnalyzer::PruneTransitiveDependencies() {
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
  const auto& original_deps = scc_.GetSCCDeps();
  if (original_deps.find(component_idx) == original_deps.end()) {
    return;
  }

  // Recursively visit each neighbor (dependency)
  for (auto neighbor : original_deps.at(component_idx)) {
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
    for (size_t i = 0; i < scc_.GetSCCComponents().size(); ++i) {
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

std::string DependencyAnalyzer::GenerateMermaidGraph(
    const std::string& keyword) const {
  return GenerateMermaidGraphWithKeyword(components_vec_,
                                         simplified_component_deps_, keyword);
}

void DependencyAnalyzer::Summary() {
  std::cout << "Mermaid Graph:\n\n";

  std::cout << GenerateMermaidGraph();

  std::cout << "```plaintext\n";
  std::cout << "Max Graph Depth: " << max_depth_ + 1 << "\n";
  std::cout << "\nTopological Sort of Files(less deps on top):\n\n";

  for (size_t depth = 0; depth <= max_depth_; ++depth) {
    for (const auto& component_idx : depth_to_component_idx_map_[depth]) {
      std::cout << "[" << depth << "]: " << components_vec_[component_idx].name
                << "\n";
    }
  }

  std::cout << "\nFile Dependencies:\n\n";
  for (const auto& [file, deps] : file_deps_) {
    std::cout << file << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }
  std::cout << "```\n";
}

std::string GenerateMermaidGraphWithKeyword(
    const std::vector<SCCComponent>& components_vec,
    const std::unordered_map<int, std::set<int>>& component_deps,
    const std::string& keyword) {
  // When keyword is empty, it basically generates the whole graph
  // otherwise, it should only prints out partial graph that is related to the
  // keyword
  std::stringstream mermaid;
  std::cout << "```mermaid\n";
  mermaid << "graph LR\n";

  // Helper function to get the name of a component
  auto get_name = [&](int component_idx) {
    if (components_vec[component_idx].members.size() > 1) {
      return "SCC_" + std::to_string(component_idx);
    } else {
      return components_vec[component_idx].name;
    }
  };

  // Track visited nodes to avoid duplicate entries
  std::unordered_set<int> visited;

  std::function<void(int)> draw_subgraph = [&](int component_idx) {
    if (visited.find(component_idx) != visited.end()) {
      return;  // Skip already processed nodes
    }
    visited.insert(component_idx);

    // Draw the current component
    if (components_vec[component_idx].members.size() > 1) {
      mermaid << "    " << get_name(component_idx) << "_contains[\""
              << get_name(component_idx) << " contains:<br/><br/>";
      for (const auto& file : components_vec[component_idx].members) {
        mermaid << file << "<br/>";
      }
      mermaid << "\"]\n";
    }

    mermaid << "    " << get_name(component_idx) << "\n";

    // Draw edges to dependent components
    if (component_deps.find(component_idx) != component_deps.end()) {
      for (const auto& to_component : component_deps.at(component_idx)) {
        if (component_idx != to_component) {
          mermaid << "    " << get_name(component_idx) << " --> "
                  << get_name(to_component) << "\n";
          draw_subgraph(to_component);  // Recursively draw dependencies
        }
      }
    }
  };

  // Find the components that match the keyword and start drawing from there
  for (size_t i = 0; i < components_vec.size(); ++i) {
    if (components_vec[i].name.find(keyword) != std::string::npos) {
      draw_subgraph(i);  // Draw subgraph rooted at this component
    }
  }

  mermaid << "```\n";
  return mermaid.str();
}
