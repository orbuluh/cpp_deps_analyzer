#include "dependency_analyzer.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

const std::set<std::string> DependencyAnalyzer::empty_set_;

DependencyAnalyzer::DependencyAnalyzer(const std::vector<File>& files)
    : files_(files) {}

void DependencyAnalyzer::AnalyzeDependencies() {
  BuildFileDependencies();
  BuildClassDependencies();
}

void DependencyAnalyzer::PrintDependencies() {
  std::cout << "File Dependencies:\n";
  for (const auto& [file, deps] : file_dependencies_) {
    std::cout << file << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }

  std::cout << "\nClass Dependencies:\n";
  for (const auto& [class_name, deps] : class_dependencies_) {
    std::cout << class_name << " depends on:\n";
    for (const auto& dep : deps) {
      std::cout << "  " << dep << "\n";
    }
  }

  std::cout << "\nTopological Sort of Files:\n";
  auto sorted_files = TopologicalSort(file_dependencies_);
  for (const auto& file : sorted_files) {
    std::cout << file << "\n";
  }
}

void DependencyAnalyzer::BuildFileDependencies() {
  for (const auto& file : files_) {
    for (const auto& header : file.included_headers) {
      file_dependencies_[file.name].insert(header);
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
    const std::string& node,
    const std::unordered_map<std::string, std::set<std::string>>& dependencies,
    std::set<std::string>& visited, std::vector<std::string>& result) const {
  visited.insert(node);

  if (dependencies.find(node) != dependencies.end()) {
    for (const auto& neighbor : dependencies.at(node)) {
      if (visited.find(neighbor) == visited.end()) {
        Dfs(neighbor, dependencies, visited, result);
      }
    }
  }

  result.push_back(node);
}

std::optional<const std::set<std::string>*>
DependencyAnalyzer::GetFileDependenciesFor(const std::string& file) const {
  auto it = file_dependencies_.find(file);
  if (it != file_dependencies_.end()) {
    return &(it->second);
  }
  return std::nullopt;
}

std::optional<const std::set<std::string>*>
DependencyAnalyzer::GetClassDependenciesFor(
    const std::string& class_name) const {
  auto it = class_dependencies_.find(class_name);
  if (it != class_dependencies_.end()) {
    return &(it->second);
  }
  return std::nullopt;
}