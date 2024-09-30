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
  DependencyAnalyzer(const std::vector<File>& files);
  void AnalyzeDependencies();
  void PrintDependencies();

  const std::unordered_map<std::string, std::set<std::string>>&
  GetFileDependencies() const {
    return file_dependencies_;
  }

  std::vector<std::string> GetTopologicallySortedFiles() const;

  std::optional<const std::set<std::string>*> GetFileDependenciesFor(
      std::string_view file) const;

  std::string GenerateMermaidGraph() const;
  int GetMaxGraphDepth() const;

  void PrintMaxDepthOrCycles() const;

 private:
  std::vector<File> files_;
  std::unordered_map<std::string, std::set<std::string>> file_dependencies_;

  std::vector<std::vector<std::string>> strongly_connected_components_;
  std::vector<std::string> topoplogical_sorted_files_;
  std::unordered_map<std::string, int> depth_map_;
  std::unordered_map<int, std::vector<std::string>> depth_to_nodes_map_;
  int max_depth_ = 0;
  // say A -> {B, C}, B -> {C} in file_dependencies_
  // we will remove the transisive deps of C in A
  // So it becomes A -> {B}, B -> {C}
  // this is to simplified the mermaid graph
  std::unordered_map<std::string, std::set<std::string>>
      simplified_file_dependencies_;

  void BuildFileDependencies();
  void FindStronglyConnectedComponents();
  void TarjanSCC(const std::string& node, std::vector<std::string>& stack,
                 std::map<std::string, int>& index,
                 std::map<std::string, int>& lowlink,
                 std::map<std::string, bool>& on_stack, int& index_counter);
  void TopologicalSort();
  void Dfs(std::string_view node, std::set<std::string>& visited);
  void BuildMinDepthRelation();
  std::string EscapeFileName(std::string_view fileName) const;

  static const std::set<std::string> empty_set_;

  int CalculateMaxDepth(
      const std::string& file,
      std::unordered_map<std::string, int>& depth_cache) const;

  bool HasCycles() const;
  std::vector<std::vector<std::string>> FindAllCycles() const;
  void DfsForCycles(const std::string& node, std::vector<std::string>& path,
                    std::set<std::string>& visited,
                    std::vector<std::vector<std::string>>& cycles) const;

  void PruneTransitiveDependencies();
  void CollectTransitiveDependencies(
      const std::string& node,
      std::unordered_set<std::string>& reachable_nodes);
};