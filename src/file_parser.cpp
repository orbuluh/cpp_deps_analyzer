#include "file_parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

FileParser::FileParser(const std::string& directory) : directory_(directory) {}

std::vector<File> FileParser::ParseFiles() {
  std::vector<File> files;
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(directory_)) {
    if (entry.is_regular_file() &&
        (std::regex_match(entry.path().extension().string(),
                          std::regex(".cpp|.h", std::regex_constants::icase)) &&
         !std::regex_search(
             entry.path().filename().string(),
             std::regex("test|mock", std::regex_constants::icase)))) {
      files.push_back(ParseFile(entry.path().string()));
    }
  }
  return files;
}

File FileParser::ParseFile(const std::string& file_path) {
  File file;
  file.name = std::filesystem::path(file_path).filename().string();

  std::ifstream in_file(file_path);
  std::string line;
  std::regex include_regex(R"(#include\s*[<"](.+)[>"])");
  std::regex class_regex(R"(class\s+(\w+))");

  while (std::getline(in_file, line)) {
    std::smatch match;
    if (std::regex_search(line, match, include_regex)) {
      std::string header = match[1].str();
      bool has_extension = header.find(".h") != std::string::npos ||
                           header.find(".hpp") != std::string::npos;
      if (has_extension) {
        file.included_headers.push_back(header);
      }
    }
    if (std::regex_search(line, match, class_regex)) {
      file.defined_classes.push_back(match[1]);
    }
  }

  return file;
}