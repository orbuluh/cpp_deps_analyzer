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
  FileParser(std::string_view directory);
  std::vector<File> ParseFiles();

 private:
  std::string directory_;
  File ParseFile(std::string_view file_path);
};