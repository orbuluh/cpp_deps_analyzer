#pragma once
#include <functional>
#include <optional>
#include <set>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "file_parser.h"

class DependencyAnalyzer {
 public:
  DependencyAnalyzer(const std::vector<File>& files);
  void AnalyzeDependencies();
  void PrintDependencies();

  // Update these getter methods
  const std::unordered_map<std::string, std::set<std::string>>&
  GetFileDependencies() const {
    return file_dependencies_;
  }
  const std::unordered_map<std::string, std::set<std::string>>&
  GetClassDependencies() const {
    return class_dependencies_;
  }
  std::vector<std::string> GetTopologicallySortedFiles() const;

  std::optional<const std::set<std::string>*> GetFileDependenciesFor(
      std::string_view file) const;

  std::optional<const std::set<std::string>*> GetClassDependenciesFor(
      std::string_view class_name) const;

  std::string GenerateMermaidGraph() const;

 private:
  std::vector<File> files_;
  std::unordered_map<std::string, std::set<std::string>> file_dependencies_;
  std::unordered_map<std::string, std::set<std::string>> class_dependencies_;

  void BuildFileDependencies();
  void BuildClassDependencies();
  std::vector<std::string> TopologicalSort(
      const std::unordered_map<std::string, std::set<std::string>>&
          dependencies) const;
  void Dfs(std::string_view node,
           const std::unordered_map<std::string, std::set<std::string>>&
               dependencies,
           std::set<std::string>& visited,
           std::vector<std::string>& result) const;
  std::string EscapeFileName(std::string_view fileName) const;

  static const std::set<std::string> empty_set_;
};