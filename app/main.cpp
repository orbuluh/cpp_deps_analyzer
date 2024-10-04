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
    std::string targeted_direcotry = argv[i];
    parser.ParseFilesUnder(targeted_direcotry);
  }
  const std::vector<File>& files = parser.GetParsedFiles();

  DependencyAnalyzer analyzer(files);
  analyzer.Summary();

  std::string keyword;
  std::cout << "Enter keyword for subgraph generation" << std::endl;

  // Loop waiting for user input
  while (true) {
    std::cout << "(please enter keyword...): ";
    std::getline(std::cin, keyword);
    // Respond to the input
    std::cout << "Generate subgraph related to " << keyword << std::endl;
    std::cout << analyzer.GenerateMermaidGraph(keyword) << std::endl;
  }

  return 0;
}