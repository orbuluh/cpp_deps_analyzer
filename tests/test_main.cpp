#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>

#include "dependency_analyzer.h"
#include "file_parser.h"

class FileParserTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a temporary directory for test files
    temp_dir_ =
        std::filesystem::temp_directory_path() / "cpp_dependency_analyzer_test";
    std::filesystem::create_directory(temp_dir_);
  }

  void TearDown() override {
    // Remove the temporary directory and its contents
    std::filesystem::remove_all(temp_dir_);
  }

  void CreateTestFile(const std::string& filename, const std::string& content) {
    std::ofstream file(temp_dir_ / filename);
    file << content;
  }

  std::filesystem::path temp_dir_;
};

TEST_F(FileParserTest, ParsesFileCorrectly) {
  CreateTestFile("dep2.h", R"(
        #include <iostream>
        struct Dep2 {};
    )");

  CreateTestFile("dep.h", R"(
        #include "dep2.h"
        struct Dep {};
    )");

  CreateTestFile("lib.cpp", R"(
        #include <iostream>
        #include "dep.h"

        struct LibType1 {};
        class LibType2 {};
        class LibType3 {};
        struct LibType4 {};
    )");

  FileParser parser;
  parser.ParseFilesUnder(temp_dir_.string());
  const std::vector<File>& files = parser.GetParsedFiles();

  ASSERT_EQ(files.size(), 3);

  auto get_file = [&files](const std::string& expected_name) {
    for (const auto& file : files) {
      if (file.name == expected_name) {
        return std::optional<File>(file);
      }
    }
    return std::optional<File>{};
  };

  auto lib_cpp_file = get_file("lib.cpp");
  EXPECT_TRUE(lib_cpp_file);
  EXPECT_EQ(lib_cpp_file.value().included_headers.size(), 1);
  EXPECT_EQ(lib_cpp_file.value().included_headers[0], "dep.h");
  EXPECT_EQ(lib_cpp_file.value().defined_classes.size(), 4);
  EXPECT_EQ(lib_cpp_file.value().defined_classes[0], "LibType1");
  EXPECT_EQ(lib_cpp_file.value().defined_classes[1], "LibType2");
  EXPECT_EQ(lib_cpp_file.value().defined_classes[2], "LibType3");
  EXPECT_EQ(lib_cpp_file.value().defined_classes[3], "LibType4");
}

class DependencyAnalyzerTest : public ::testing::Test {
 protected:
  std::vector<File> CreateTestFiles() {
    return {
        {"file1.cpp", {"header1.h", "header2.h"}, {"Class1"}},
        {"file2.cpp", {"header2.h", "header3.h"}, {"Struct2"}},
        {"header1.h", {"header4.h"}, {}},
        {"header2.h", {}, {"Class2"}},
        {"header3.h", {}, {"Struct3"}},
        {"header4.h", {}, {"Class4", "Struct5"}},
    };
  }
};

TEST_F(DependencyAnalyzerTest, AnalyzesDependenciesCorrectly) {
  std::vector<File> files = CreateTestFiles();
  DependencyAnalyzer analyzer(files);
  analyzer.AnalyzeDependencies();
  // analyzer.PrintDependencies();

  // Test file dependencies
  auto file1_deps = analyzer.GetFileDependenciesFor("file1.cpp");
  ASSERT_TRUE(file1_deps.has_value());
  EXPECT_EQ(file1_deps.value()->size(), 2);

  auto file2_deps = analyzer.GetFileDependenciesFor("file2.cpp");
  ASSERT_TRUE(file2_deps.has_value());
  EXPECT_EQ(file2_deps.value()->size(), 2);

  // Test non-existent dependencies
  auto non_existent_file_deps =
      analyzer.GetFileDependenciesFor("non_existent.cpp");
  EXPECT_FALSE(non_existent_file_deps.has_value());
}

TEST_F(DependencyAnalyzerTest, TopologicalSortWorks) {
  std::vector<File> files = CreateTestFiles();
  DependencyAnalyzer analyzer(files);
  analyzer.AnalyzeDependencies();

  std::vector<std::string> sorted_files =
      analyzer.GetTopologicallySortedFiles();
  EXPECT_EQ(sorted_files.size(), 6);  // 2 cpp files + 4 headers
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}