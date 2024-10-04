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

class SCCBuilderTest : public FileParserTest {};

TEST_F(SCCBuilderTest, NoSccCase) {
  std::vector<File> files = {
      {"A.cpp", {"A.h", "B.h", "C.h"}},
      {"B.cpp", {"B.h", "C.h"}},
      {"C.cpp", {"C.h"}},
      {"A.h", {}},
      {"B.h", {}},
      {"C.h", {}},
  };

  const auto& file_deps{BuildFileDependencies(files)};

  ASSERT_EQ(file_deps.size(), 3);
  ASSERT_EQ(file_deps.at("A").size(), 2);
  ASSERT_EQ(file_deps.at("B").size(), 1);
  ASSERT_EQ(file_deps.at("C").size(), 0);

  SCCBuilder scc{file_deps};
  const auto& scc_vec{scc.GetSCCComponents()};

  ASSERT_EQ(scc_vec.size(), 3);
}

TEST_F(SCCBuilderTest, SccNoDepsCase) {
  // Create files with circular dependencies (SCC scenario)
  std::vector<File> files = {
      {"A.cpp", {"B.h"}},  // A.cpp depends on B.h
      {"B.cpp", {"C.h"}},  // B.cpp depends on C.h
      {"C.cpp", {"A.h"}},  // C.cpp depends on A.h (cycle created)
      {"D.cpp", {"D.h"}}, {"A.h", {}}, {"B.h", {}}, {"C.h", {}}, {"D.h", {}},
  };

  const auto& file_deps{BuildFileDependencies(files)};

  ASSERT_EQ(file_deps.size(), 4);
  ASSERT_EQ(file_deps.at("A").size(), 1);
  ASSERT_EQ(file_deps.at("B").size(), 1);
  ASSERT_EQ(file_deps.at("C").size(), 1);
  ASSERT_EQ(file_deps.at("D").size(), 0);

  SCCBuilder scc{file_deps};
  const auto& scc_vec{scc.GetSCCComponents()};

  // There should be exactly 2 SCC, one containing A, B, C, the other contain D
  ASSERT_EQ(scc_vec.size(), 2);
  const auto& scc1 = scc_vec[0].members;
  ASSERT_EQ(scc1.size(), 3);
  ASSERT_TRUE(std::find(scc1.begin(), scc1.end(), "A") != scc1.end());
  ASSERT_TRUE(std::find(scc1.begin(), scc1.end(), "B") != scc1.end());
  ASSERT_TRUE(std::find(scc1.begin(), scc1.end(), "C") != scc1.end());
  const auto& scc2 = scc_vec[1].members;
  ASSERT_EQ(scc2.size(), 1);
  ASSERT_EQ(scc2[0], "D");
}

TEST_F(SCCBuilderTest, TwoSccWithDep) {
  // Create files where SCC 1 depends on SCC 2
  std::vector<File> files = {
      {"A.cpp", {"B.h", "C.h"}},  // A.cpp depends on B.h (forms SCC 1 with B)
      {"B.cpp", {"A.h"}},  // B.cpp depends on A.h (cycle A -> B -> A, SCC 1)
      {"C.cpp", {"D.h"}},  // C.cpp depends on D.h (forms SCC 2 with D)
      {"D.cpp", {"C.h"}},  // D.cpp depends on C.h (cycle C -> D -> C, SCC 2)
      {"A.h", {}},
      {"B.h", {}},
      {"C.h", {}},
      {"D.h", {}},
  };

  DependencyAnalyzer analyzer(files);

  analyzer.Summary();

  const auto& deps = analyzer.GetFileDependencies();
  ASSERT_EQ(deps.at("A").size(), 2);
  ASSERT_EQ(deps.at("B").size(), 1);
  ASSERT_EQ(deps.at("C").size(), 1);
  ASSERT_EQ(deps.at("D").size(), 1);

  const auto& sccs = analyzer.GetStronglyConnectedComponents();

  // There should be exactly 2 SCCs
  ASSERT_EQ(sccs.size(), 2);

  // SCC 1: Contains A and B (cycle between them)
  // SCC 2: Contains C and D (cycle between them)

  if (sccs[0].contains("A")) {
    ASSERT_TRUE(sccs[0].contains("B"));
    ASSERT_TRUE(sccs[1].contains("C"));
    ASSERT_TRUE(sccs[1].contains("D"));
  } else {
    ASSERT_TRUE(sccs[0].contains("C"));
    ASSERT_TRUE(sccs[0].contains("D"));
    ASSERT_TRUE(sccs[1].contains("A"));
    ASSERT_TRUE(sccs[1].contains("B"));
  }

  const auto& sorted_sccs = analyzer.GetTopologicalSortedSCCs();

  // Verify the topological order (SCC 2 should appear before SCC 1 since SCC 1
  // depends on SCC 2)
  ASSERT_EQ(sorted_sccs.size(), 2);
  ASSERT_TRUE(std::find(sorted_sccs.begin(), sorted_sccs.end(), 1) <
              std::find(sorted_sccs.begin(), sorted_sccs.end(), 0));
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}