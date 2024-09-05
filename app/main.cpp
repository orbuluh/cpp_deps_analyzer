#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "dependency_analyzer.h"
#include "file_parser.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <directory>" << std::endl;
    return 1;
  }

  std::string directory = argv[1];
  FileParser parser(directory);
  std::vector<File> files = parser.ParseFiles();

  DependencyAnalyzer analyzer(files);
  analyzer.AnalyzeDependencies();
  analyzer.PrintDependencies();

  return 0;
}