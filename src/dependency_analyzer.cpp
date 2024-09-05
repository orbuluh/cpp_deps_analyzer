#include "dependency_analyzer.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

const std::set<std::string> DependencyAnalyzer::empty_set_;

DependencyAnalyzer::DependencyAnalyzer(const std::vector<File>& files)
    : files_(files) {}

void DependencyAnalyzer::AnalyzeDependencies() {
  BuildFileDependencies();
  BuildClassDependencies();
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
}

void DependencyAnalyzer::BuildFileDependencies() {
  for (const auto& file : files_) {
    for (const auto& header : file.included_headers) {
      // Find the full path of the header file
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

// Add this new method implementation
std::string DependencyAnalyzer::GenerateMermaidGraph() const {
  std::stringstream mermaid;
  mermaid << "graph TD\n";

  for (const auto& [file, deps] : file_dependencies_) {
    for (const auto& dep : deps) {
      mermaid << "    " << EscapeFileName(file) << " --> "
              << EscapeFileName(dep) << "\n";
    }
  }

  return mermaid.str();
}

// Add this helper method to escape special characters in file names
std::string DependencyAnalyzer::EscapeFileName(
    const std::string& fileName) const {
  std::string escaped = fileName;
  std::replace(escaped.begin(), escaped.end(), '/', '_');
  std::replace(escaped.begin(), escaped.end(), '\\', '_');
  std::replace(escaped.begin(), escaped.end(), '.', '_');
  std::replace(escaped.begin(), escaped.end(), '-', '_');
  return escaped;
}
