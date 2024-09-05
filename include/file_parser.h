#pragma once
#include <string>
#include <vector>

struct File {
  std::string name;
  std::vector<std::string> included_headers;
  std::vector<std::string> defined_classes;
};

class FileParser {
 public:
  FileParser(const std::string& directory);
  std::vector<File> ParseFiles();

 private:
  std::string directory_;
  File ParseFile(const std::string& file_path);
};