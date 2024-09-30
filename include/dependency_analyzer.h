#pragma once

#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "file_parser.h"

class DependencyAnalyzer {
 public:
  using SccIdx = int;

  DependencyAnalyzer(const std::vector<File>& files);
  void PrintDependencies();
  void PrintStronglyConnectedComponents() const;

  std::string GenerateMermaidGraph();

 private:
  std::vector<File> files_;
  int max_depth_;
  std::unordered_map<std::string, std::set<std::string>> file_deps_;
  std::vector<std::vector<std::string>> strongly_connected_components_;
  std::vector<std::string> scc_name_;
  std::unordered_map<std::string, SccIdx> file_to_component_;
  std::unordered_map<SccIdx, std::set<SccIdx>> component_deps_;
  std::vector<SccIdx> topoplogical_sorted_sccs_;
  std::unordered_map<SccIdx, int> depth_map_;
  std::unordered_map<int, std::vector<SccIdx>> depth_to_component_idx_map_;
  // say A -> {B, C}, B -> {C} in file_deps_
  // we will remove the transisive deps of C in A
  // So it becomes A -> {B}, B -> {C}
  // this is to simplified the mermaid graph
  std::unordered_map<int, std::set<SccIdx>> simplified_component_deps_;

 public:
  // Getters for const reference access to private members
  const std::unordered_map<std::string, std::set<std::string>>&
  GetFileDependencies() const {
    return file_deps_;
  }
  const std::vector<std::vector<std::string>>& GetStronglyConnectedComponents()
      const {
    return strongly_connected_components_;
  }
  const std::vector<std::string>& GetSCCName() const { return scc_name_; }
  const std::unordered_map<std::string, SccIdx>& GetFileToComponent() const {
    return file_to_component_;
  }
  const std::unordered_map<int, std::set<SccIdx>>& GetComponentDependencies()
      const {
    return component_deps_;
  }
  const std::vector<SccIdx>& GetTopologicalSortedSCCs() const {
    return topoplogical_sorted_sccs_;
  }
  const std::unordered_map<SccIdx, int>& GetDepthMap() const {
    return depth_map_;
  }
  const std::unordered_map<int, std::vector<SccIdx>>&
  GetDepthToComponentIdxMap() const {
    return depth_to_component_idx_map_;
  }
  const std::unordered_map<int, std::set<SccIdx>>&
  GetSimplifiedComponentDependencies() const {
    return simplified_component_deps_;
  }

 private:
  void BuildFileDependencies();
  void BuildSCC();
  void BuildSCCNames();
  void TarjanSCC(const std::string& node, std::vector<std::string>& stack,
                 std::map<std::string, int>& index,
                 std::map<std::string, int>& lowlink,
                 std::map<std::string, bool>& on_stack, int& index_counter);
  void BuildSCCDependencies();
  void TopologicalSortSCCDependencies();
  void Dfs(SccIdx component_idx, std::set<SccIdx>& visited);

  void PruneTransitiveDependencies();
  void CollectTransitiveDependencies(
      SccIdx component_idx, std::unordered_set<SccIdx>& reachable_indices);

  void BuildMinDepthRelation();
};