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
  using DepEdges = std::unordered_map<std::string, std::set<std::string>>;

  DependencyAnalyzer(const std::vector<File>& files);

  void AnalyzeDependencies();
  void PrintDependencies();
  void PrintMaxDepthOrCycles() const;

  const DepEdges& GetFileDependencies() const { return file_dependencies_; }
  std::vector<std::string> GetTopologicallySortedFiles() const;
  std::optional<const std::set<std::string>*> GetFileDependenciesFor(
      std::string_view file) const;

  std::string GenerateMermaidGraph();

 private:
  std::vector<File> files_;
  int max_depth_;
  DepEdges file_dependencies_;
  std::vector<std::vector<std::string>> cycles_;
  std::vector<std::vector<std::string>> strongly_connected_components_;
  std::unordered_map<std::string, int> file_to_component_;
  std::vector<std::string> topoplogical_sorted_files_;
  std::unordered_map<std::string, int> depth_map_;
  std::unordered_map<int, std::vector<std::string>> depth_to_nodes_map_;
  // say A -> {B, C}, B -> {C} in file_dependencies_
  // we will remove the transisive deps of C in A
  // So it becomes A -> {B}, B -> {C}
  // this is to simplified the mermaid graph
  DepEdges simplified_file_dependencies_;

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

  int CalculateMaxDepth(
      const std::string& file,
      std::unordered_map<std::string, int>& depth_cache) const;

  bool HasCycles() const;
  void FindAllCycles();
  void DfsForCycles(const std::string& node, std::vector<std::string>& path,
                    std::set<std::string>& visited);

  void PruneTransitiveDependencies();
  void CollectTransitiveDependencies(
      const std::string& node,
      std::unordered_set<std::string>& reachable_nodes);
};