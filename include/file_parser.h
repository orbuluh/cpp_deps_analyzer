#pragma once
#include <string>
#include <string_view>
#include <vector>

struct File {
  std::string name;
  std::vector<std::string> included_headers;
  std::vector<std::string> defined_classes;
};

class FileParser {
 public:
  void ParseFilesUnder(std::string_view directory);
  const std::vector<File>& GetParsedFiles() const;

 private:
  std::vector<File> parsed_files_;
  File ParseFile(std::string_view file_path, std::string_view relative_to_path);
};