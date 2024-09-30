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
    std::cerr << "Usage: " << argv[0] << " <file1> <file2> ..." << std::endl;
    return 1;
  }

  FileParser parser;

  for (int i = 1; i < argc; ++i) {
    std::string file = argv[i];
    parser.ParseFilesUnder(file);  // Parse each file individually
  }

  const std::vector<File>& files = parser.GetParsedFiles();

  DependencyAnalyzer analyzer(files);
  analyzer.PrintDependencies();

  return 0;
}