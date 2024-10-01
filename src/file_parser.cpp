#include "file_parser.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

const std::vector<File>& FileParser::GetParsedFiles() const {
  return parsed_files_;
}

void FileParser::ParseFilesUnder(std::string_view directory) {
  std::filesystem::path base_path = std::filesystem::absolute(directory);
  for (const auto& entry :
       std::filesystem::recursive_directory_iterator(base_path)) {
    if (entry.is_regular_file() &&
        (std::regex_match(entry.path().extension().string(),
                          std::regex(".c|.cpp|.cu|.h|.hpp|.hu",
                                     std::regex_constants::icase)) &&
         !std::regex_search(
             entry.path().filename().string(),
             std::regex("test|mock", std::regex_constants::icase)))) {
      // The file is already inside the provided directory, so we can add it
      parsed_files_.emplace_back(ParseFile(entry.path().string(), directory));
    } else {
      std::cerr << "Skipping " << entry.path().filename().string() << '\n';
    }
  }
}

File FileParser::ParseFile(std::string_view file_path,
                           std::string_view relative_to_path) {
  File file;
  file.name = std::filesystem::relative(file_path, relative_to_path).string();

  std::ifstream in_file(file_path.data());
  std::string line;
  std::regex include_regex(R"(^\s*#include\s*[<"](.+)[>"])");
  std::regex class_regex(R"(class\s+(\w+)|struct\s+(\w+))");

  while (std::getline(in_file, line)) {
    std::smatch match;
    if (std::regex_search(line, match, include_regex)) {
      std::string header = match[1].str();
      bool has_extension = header.find(".h") != std::string::npos ||
                           header.find(".hpp") != std::string::npos ||
                           header.find(".hu") != std::string::npos;
      if (has_extension) {
        file.included_headers.push_back(header);
      }
    }
    if (std::regex_search(line, match, class_regex)) {
      if (match[1].matched) {  // matched class
        file.defined_classes.push_back(match[1]);
      }
      if (match[2].matched) {  // matched struct
        file.defined_classes.push_back(match[2]);
      }
    }
  }

  return file;
}