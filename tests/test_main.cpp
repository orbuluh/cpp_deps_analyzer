#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

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
  CreateTestFile("test.cpp", R"(
        #include <iostream>
        #include "test.h"
        class TestClass {
        };
    )");

  FileParser parser(temp_dir_.string());
  std::vector<File> files = parser.ParseFiles();

  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].name, "test.cpp");
  EXPECT_EQ(files[0].included_headers.size(), 1);
  EXPECT_EQ(files[0].included_headers[0], "test.h");
  EXPECT_EQ(files[0].defined_classes.size(), 1);
  EXPECT_EQ(files[0].defined_classes[0], "TestClass");
}

class DependencyAnalyzerTest : public ::testing::Test {
 protected:
  std::vector<File> CreateTestFiles() {
    File file1{"file1.cpp", {"header1.h", "header2.h"}, {"Class1"}};
    File file2{"file2.cpp", {"header2.h", "header3.h"}, {"Class2"}};
    return {file1, file2};
  }
};

TEST_F(DependencyAnalyzerTest, AnalyzesDependenciesCorrectly) {
  std::vector<File> files = CreateTestFiles();
  DependencyAnalyzer analyzer(files);
  analyzer.AnalyzeDependencies();

  // Test file dependencies
  auto file1_deps = analyzer.GetFileDependenciesFor("file1.cpp");
  ASSERT_TRUE(file1_deps.has_value());
  EXPECT_EQ(file1_deps.value()->size(), 2);

  auto file2_deps = analyzer.GetFileDependenciesFor("file2.cpp");
  ASSERT_TRUE(file2_deps.has_value());
  EXPECT_EQ(file2_deps.value()->size(), 2);

  // Test class dependencies
  auto class1_deps = analyzer.GetClassDependenciesFor("Class1");
  ASSERT_TRUE(class1_deps.has_value());
  EXPECT_EQ(class1_deps.value()->size(), 2);

  auto class2_deps = analyzer.GetClassDependenciesFor("Class2");
  ASSERT_TRUE(class2_deps.has_value());
  EXPECT_EQ(class2_deps.value()->size(), 2);

  // Test non-existent dependencies
  auto non_existent_file_deps =
      analyzer.GetFileDependenciesFor("non_existent.cpp");
  EXPECT_FALSE(non_existent_file_deps.has_value());

  auto non_existent_class_deps =
      analyzer.GetClassDependenciesFor("NonExistentClass");
  EXPECT_FALSE(non_existent_class_deps.has_value());
}

TEST_F(DependencyAnalyzerTest, TopologicalSortWorks) {
  std::vector<File> files = CreateTestFiles();
  DependencyAnalyzer analyzer(files);
  analyzer.AnalyzeDependencies();

  std::vector<std::string> sorted_files =
      analyzer.GetTopologicallySortedFiles();
  EXPECT_EQ(sorted_files.size(), 5);  // 2 cpp files + 3 headers
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}