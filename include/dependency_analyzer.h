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

using StrDepMap = std::unordered_map<std::string, std::set<std::string>>;

StrDepMap BuildFileDependencies(const std::vector<File>& files);

using SccIdx = int;
using SccDepMap = std::unordered_map<SccIdx, std::set<SccIdx>>;

struct SCCComponent {
  std::string name;                  // Concatenated name of SCC members
  std::vector<std::string> members;  // Actual members of the SCC

  SCCComponent(const std::string& name, const std::vector<std::string>& members)
      : name(name), members(members) {}
  bool contains(std::string_view file) const {
    return std::find(members.begin(), members.end(), file) != members.end();
  }
};

class SCCBuilder {
 public:
  explicit SCCBuilder(const StrDepMap& file_deps);

  std::optional<SccIdx> GetComponentIndex(const std::string& file) const;
  const std::vector<SCCComponent>& GetSCCComponents() const {
    return scc_components_;
  };
  const SccDepMap& GetSCCDeps() const { return component_deps_; };
  std::string ToDescription() const;

 private:
  const StrDepMap& file_deps_;  // Store file dependencies
  std::unordered_map<std::string, SccIdx> file_to_component_;
  std::vector<SCCComponent> scc_components_;
  SccDepMap component_deps_;

 private:
  void BuildSCC();
  void BuildSCCNames();
  void BuildSCCDependencies();
  void TarjanSCC(const std::string& node, std::vector<std::string>& stack,
                 std::map<std::string, int>& index,
                 std::map<std::string, int>& lowlink,
                 std::map<std::string, bool>& on_stack, int& index_counter);
};

class DependencyAnalyzer {
 public:
  DependencyAnalyzer(const std::vector<File>& files);
  void PrintDependencies();

  std::string GenerateMermaidGraph();

 private:
  int max_depth_;
  StrDepMap file_deps_;
  SCCBuilder scc_;
  const std::vector<SCCComponent>& components_vec_;
  // say A -> {B, C}, B -> {C} in file_deps_
  // we will remove the transisive deps of C in A
  // So it becomes A -> {B}, B -> {C}
  // this is to simplified the mermaid graph
  SccDepMap simplified_component_deps_;
  std::vector<SccIdx> topoplogical_sorted_sccs_;
  std::unordered_map<SccIdx, int> depth_map_;
  std::unordered_map<int, std::vector<SccIdx>> depth_to_component_idx_map_;

 public:
  // Getters for const reference access to private members
  const std::vector<SccIdx>& GetTopologicalSortedSCCs() const {
    return topoplogical_sorted_sccs_;
  }
  const StrDepMap& GetFileDependencies() const { return file_deps_; }
  const std::vector<SCCComponent>& GetStronglyConnectedComponents() const {
    return components_vec_;
  }

 private:
  void PruneTransitiveDependencies();
  void CollectTransitiveDependencies(
      SccIdx component_idx, std::unordered_set<SccIdx>& reachable_indices);
  void TopologicalSortSCCDependencies();
  void Dfs(SccIdx component_idx, std::set<SccIdx>& visited);
  void BuildMinDepthRelation();
};