#include <catch2/catch_test_macros.hpp>

#include <immer-archive/xxhash/xxhash.hpp>

#include <fmt/format.h>
#include <immer-archive/champ/traits.hpp>
#include <xxhash.h>

#include <nlohmann/json.hpp>
#include <test/utils.hpp>

namespace {
const auto gen_map = [](auto map, int count) {
    for (int i = 0; i < count; ++i) {
        map = std::move(map).set(fmt::format("key_{}_", i),
                                 fmt::format("value_{}_", i));
    }
    return map;
};
}

TEST_CASE("Test hash strings")
{
    auto str = std::string{};
    REQUIRE(immer_archive::xx_hash<std::string>{}(str) == 3244421341483603138);
    REQUIRE(XXH3_64bits(str.c_str(), str.size()) == 3244421341483603138);

    str = "hello";
    REQUIRE(immer_archive::xx_hash<std::string>{}(str) ==
            10760762337991515389UL);
    REQUIRE(XXH3_64bits(str.c_str(), str.size()) == 10760762337991515389UL);
}

TEST_CASE("Test loading a big map saved on macOS with std::hash", "[.macos]")
{
    using Container = immer::map<std::string, std::string>;

    const auto set          = gen_map(Container{}, 200);
    const auto [ar, set_id] = immer_archive::champ::save_to_archive(set, {});
    const auto ar_str       = test::to_json(ar);
    // REQUIRE(ar_str == "");
    const auto expected_json = R"({
  "value0": {
    "nodes": {
      "inners": [
        {
          "key": 15,
          "value": {
            "values": [
              {"first": "key_33_", "second": "value_33_"},
              {"first": "key_183_", "second": "value_183_"},
              {"first": "key_122_", "second": "value_122_"}
            ],
            "children": [16],
            "nodemap": 32,
            "datamap": 73984
          }
        },
        {
          "key": 16,
          "value": {
            "values": [
              {"first": "key_156_", "second": "value_156_"},
              {"first": "key_37_", "second": "value_37_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1088
          }
        },
        {
          "key": 17,
          "value": {
            "values": [
              {"first": "key_130_", "second": "value_130_"},
              {"first": "key_79_", "second": "value_79_"},
              {"first": "key_180_", "second": "value_180_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 16912384
          }
        },
        {
          "key": 18,
          "value": {
            "values": [
              {"first": "key_197_", "second": "value_197_"},
              {"first": "key_157_", "second": "value_157_"},
              {"first": "key_85_", "second": "value_85_"},
              {"first": "key_11_", "second": "value_11_"},
              {"first": "key_0_", "second": "value_0_"},
              {"first": "key_86_", "second": "value_86_"},
              {"first": "key_104_", "second": "value_104_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1610648610
          }
        },
        {
          "key": 19,
          "value": {
            "values": [
              {"first": "key_147_", "second": "value_147_"},
              {"first": "key_125_", "second": "value_125_"},
              {"first": "key_127_", "second": "value_127_"},
              {"first": "key_58_", "second": "value_58_"},
              {"first": "key_39_", "second": "value_39_"},
              {"first": "key_149_", "second": "value_149_"},
              {"first": "key_178_", "second": "value_178_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1610884
          }
        },
        {
          "key": 20,
          "value": {
            "values": [
              {"first": "key_103_", "second": "value_103_"},
              {"first": "key_176_", "second": "value_176_"},
              {"first": "key_123_", "second": "value_123_"},
              {"first": "key_199_", "second": "value_199_"},
              {"first": "key_56_", "second": "value_56_"},
              {"first": "key_148_", "second": "value_148_"}
            ],
            "children": [21],
            "nodemap": 536870912,
            "datamap": 92275203
          }
        },
        {
          "key": 21,
          "value": {
            "values": [
              {"first": "key_169_", "second": "value_169_"},
              {"first": "key_109_", "second": "value_109_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33558528
          }
        },
        {
          "key": 22,
          "value": {
            "values": [
              {"first": "key_34_", "second": "value_34_"},
              {"first": "key_2_", "second": "value_2_"},
              {"first": "key_28_", "second": "value_28_"},
              {"first": "key_179_", "second": "value_179_"},
              {"first": "key_171_", "second": "value_171_"},
              {"first": "key_162_", "second": "value_162_"}
            ],
            "children": [23],
            "nodemap": 32768,
            "datamap": 68436040
          }
        },
        {
          "key": 23,
          "value": {
            "values": [
              {"first": "key_88_", "second": "value_88_"},
              {"first": "key_84_", "second": "value_84_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 16777220
          }
        },
        {
          "key": 24,
          "value": {
            "values": [
              {"first": "key_93_", "second": "value_93_"},
              {"first": "key_186_", "second": "value_186_"},
              {"first": "key_121_", "second": "value_121_"},
              {"first": "key_181_", "second": "value_181_"},
              {"first": "key_175_", "second": "value_175_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1610655744
          }
        },
        {
          "key": 25,
          "value": {
            "values": [
              {"first": "key_13_", "second": "value_13_"},
              {"first": "key_64_", "second": "value_64_"},
              {"first": "key_128_", "second": "value_128_"},
              {"first": "key_5_", "second": "value_5_"},
              {"first": "key_138_", "second": "value_138_"},
              {"first": "key_117_", "second": "value_117_"},
              {"first": "key_54_", "second": "value_54_"},
              {"first": "key_72_", "second": "value_72_"}
            ],
            "children": [26],
            "nodemap": 65536,
            "datamap": 378537000
          }
        },
        {
          "key": 26,
          "value": {
            "values": [
              {"first": "key_154_", "second": "value_154_"},
              {"first": "key_14_", "second": "value_14_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 268436480
          }
        },
        {
          "key": 27,
          "value": {
            "values": [
              {"first": "key_120_", "second": "value_120_"},
              {"first": "key_20_", "second": "value_20_"},
              {"first": "key_1_", "second": "value_1_"},
              {"first": "key_195_", "second": "value_195_"}
            ],
            "children": [28, 29],
            "nodemap": 4202496,
            "datamap": 25296897
          }
        },
        {
          "key": 28,
          "value": {
            "values": [
              {"first": "key_4_", "second": "value_4_"},
              {"first": "key_165_", "second": "value_165_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 134217984
          }
        },
        {
          "key": 29,
          "value": {
            "values": [
              {"first": "key_80_", "second": "value_80_"},
              {"first": "key_87_", "second": "value_87_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1074790400
          }
        },
        {
          "key": 30,
          "value": {
            "values": [
              {"first": "key_185_", "second": "value_185_"},
              {"first": "key_42_", "second": "value_42_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 49152
          }
        },
        {
          "key": 31,
          "value": {
            "values": [
              {"first": "key_136_", "second": "value_136_"},
              {"first": "key_112_", "second": "value_112_"},
              {"first": "key_29_", "second": "value_29_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1077936256
          }
        },
        {
          "key": 0,
          "value": {
            "values": [],
            "children": [
              1,
              3,
              5,
              7,
              9,
              11,
              12,
              13,
              14,
              15,
              17,
              18,
              19,
              20,
              22,
              24,
              25,
              27,
              30,
              31,
              32,
              35,
              37,
              38,
              39,
              40,
              41,
              42,
              43,
              44,
              45,
              46
            ],
            "nodemap": 4294967295,
            "datamap": 0
          }
        },
        {
          "key": 32,
          "value": {
            "values": [
              {"first": "key_97_", "second": "value_97_"},
              {"first": "key_17_", "second": "value_17_"},
              {"first": "key_61_", "second": "value_61_"},
              {"first": "key_66_", "second": "value_66_"},
              {"first": "key_19_", "second": "value_19_"},
              {"first": "key_108_", "second": "value_108_"},
              {"first": "key_141_", "second": "value_141_"},
              {"first": "key_184_", "second": "value_184_"},
              {"first": "key_187_", "second": "value_187_"}
            ],
            "children": [33],
            "nodemap": 512,
            "datamap": 10285057
          }
        },
        {
          "key": 1,
          "value": {
            "values": [
              {"first": "key_7_", "second": "value_7_"},
              {"first": "key_78_", "second": "value_78_"},
              {"first": "key_189_", "second": "value_189_"},
              {"first": "key_76_", "second": "value_76_"}
            ],
            "children": [2],
            "nodemap": 1048576,
            "datamap": 67842
          }
        },
        {
          "key": 33,
          "value": {"values": [], "children": [34], "nodemap": 2, "datamap": 0}
        },
        {
          "key": 2,
          "value": {
            "values": [
              {"first": "key_65_", "second": "value_65_"},
              {"first": "key_8_", "second": "value_8_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33
          }
        },
        {
          "key": 34,
          "value": {
            "values": [
              {"first": "key_75_", "second": "value_75_"},
              {"first": "key_100_", "second": "value_100_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2151677952
          }
        },
        {
          "key": 3,
          "value": {
            "values": [
              {"first": "key_49_", "second": "value_49_"},
              {"first": "key_77_", "second": "value_77_"},
              {"first": "key_62_", "second": "value_62_"},
              {"first": "key_119_", "second": "value_119_"},
              {"first": "key_129_", "second": "value_129_"},
              {"first": "key_105_", "second": "value_105_"},
              {"first": "key_151_", "second": "value_151_"},
              {"first": "key_144_", "second": "value_144_"}
            ],
            "children": [4],
            "nodemap": 256,
            "datamap": 140947456
          }
        },
        {
          "key": 35,
          "value": {
            "values": [
              {"first": "key_196_", "second": "value_196_"},
              {"first": "key_167_", "second": "value_167_"},
              {"first": "key_160_", "second": "value_160_"},
              {"first": "key_50_", "second": "value_50_"},
              {"first": "key_164_", "second": "value_164_"},
              {"first": "key_27_", "second": "value_27_"}
            ],
            "children": [36],
            "nodemap": 4194304,
            "datamap": 2150105155
          }
        },
        {
          "key": 4,
          "value": {
            "values": [
              {"first": "key_174_", "second": "value_174_"},
              {"first": "key_193_", "second": "value_193_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1536
          }
        },
        {
          "key": 36,
          "value": {
            "values": [
              {"first": "key_21_", "second": "value_21_"},
              {"first": "key_18_", "second": "value_18_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 8519680
          }
        },
        {
          "key": 5,
          "value": {
            "values": [
              {"first": "key_124_", "second": "value_124_"},
              {"first": "key_40_", "second": "value_40_"},
              {"first": "key_118_", "second": "value_118_"},
              {"first": "key_134_", "second": "value_134_"},
              {"first": "key_153_", "second": "value_153_"},
              {"first": "key_194_", "second": "value_194_"},
              {"first": "key_143_", "second": "value_143_"}
            ],
            "children": [6],
            "nodemap": 33554432,
            "datamap": 1704368
          }
        },
        {
          "key": 37,
          "value": {
            "values": [
              {"first": "key_95_", "second": "value_95_"},
              {"first": "key_83_", "second": "value_83_"},
              {"first": "key_170_", "second": "value_170_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2099204
          }
        },
        {
          "key": 6,
          "value": {
            "values": [
              {"first": "key_155_", "second": "value_155_"},
              {"first": "key_9_", "second": "value_9_"},
              {"first": "key_69_", "second": "value_69_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1077938176
          }
        },
        {
          "key": 38,
          "value": {
            "values": [
              {"first": "key_92_", "second": "value_92_"},
              {"first": "key_55_", "second": "value_55_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 258
          }
        },
        {
          "key": 7,
          "value": {
            "values": [
              {"first": "key_74_", "second": "value_74_"},
              {"first": "key_188_", "second": "value_188_"},
              {"first": "key_52_", "second": "value_52_"},
              {"first": "key_140_", "second": "value_140_"},
              {"first": "key_163_", "second": "value_163_"},
              {"first": "key_166_", "second": "value_166_"}
            ],
            "children": [8],
            "nodemap": 134217728,
            "datamap": 85985376
          }
        },
        {
          "key": 39,
          "value": {
            "values": [
              {"first": "key_6_", "second": "value_6_"},
              {"first": "key_60_", "second": "value_60_"},
              {"first": "key_99_", "second": "value_99_"},
              {"first": "key_150_", "second": "value_150_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 270532737
          }
        },
        {
          "key": 8,
          "value": {
            "values": [
              {"first": "key_36_", "second": "value_36_"},
              {"first": "key_106_", "second": "value_106_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1088
          }
        },
        {
          "key": 40,
          "value": {
            "values": [
              {"first": "key_48_", "second": "value_48_"},
              {"first": "key_98_", "second": "value_98_"},
              {"first": "key_131_", "second": "value_131_"},
              {"first": "key_24_", "second": "value_24_"},
              {"first": "key_68_", "second": "value_68_"},
              {"first": "key_51_", "second": "value_51_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 235012624
          }
        },
        {
          "key": 9,
          "value": {
            "values": [
              {"first": "key_111_", "second": "value_111_"},
              {"first": "key_26_", "second": "value_26_"},
              {"first": "key_192_", "second": "value_192_"},
              {"first": "key_70_", "second": "value_70_"}
            ],
            "children": [10],
            "nodemap": 134217728,
            "datamap": 200768
          }
        },
        {
          "key": 41,
          "value": {
            "values": [
              {"first": "key_173_", "second": "value_173_"},
              {"first": "key_135_", "second": "value_135_"},
              {"first": "key_142_", "second": "value_142_"},
              {"first": "key_12_", "second": "value_12_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 270532611
          }
        },
        {
          "key": 10,
          "value": {
            "values": [
              {"first": "key_45_", "second": "value_45_"},
              {"first": "key_31_", "second": "value_31_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2097160
          }
        },
        {
          "key": 42,
          "value": {
            "values": [
              {"first": "key_44_", "second": "value_44_"},
              {"first": "key_47_", "second": "value_47_"},
              {"first": "key_10_", "second": "value_10_"},
              {"first": "key_30_", "second": "value_30_"},
              {"first": "key_146_", "second": "value_146_"},
              {"first": "key_38_", "second": "value_38_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 109086728
          }
        },
        {
          "key": 11,
          "value": {
            "values": [
              {"first": "key_46_", "second": "value_46_"},
              {"first": "key_115_", "second": "value_115_"},
              {"first": "key_89_", "second": "value_89_"},
              {"first": "key_63_", "second": "value_63_"},
              {"first": "key_91_", "second": "value_91_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 5263424
          }
        },
        {
          "key": 43,
          "value": {
            "values": [
              {"first": "key_82_", "second": "value_82_"},
              {"first": "key_102_", "second": "value_102_"},
              {"first": "key_81_", "second": "value_81_"},
              {"first": "key_190_", "second": "value_190_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2164391938
          }
        },
        {
          "key": 12,
          "value": {
            "values": [
              {"first": "key_41_", "second": "value_41_"},
              {"first": "key_116_", "second": "value_116_"},
              {"first": "key_16_", "second": "value_16_"},
              {"first": "key_101_", "second": "value_101_"},
              {"first": "key_145_", "second": "value_145_"},
              {"first": "key_110_", "second": "value_110_"},
              {"first": "key_32_", "second": "value_32_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 36243840
          }
        },
        {
          "key": 44,
          "value": {
            "values": [
              {"first": "key_133_", "second": "value_133_"},
              {"first": "key_3_", "second": "value_3_"},
              {"first": "key_22_", "second": "value_22_"},
              {"first": "key_25_", "second": "value_25_"},
              {"first": "key_35_", "second": "value_35_"},
              {"first": "key_168_", "second": "value_168_"},
              {"first": "key_126_", "second": "value_126_"},
              {"first": "key_158_", "second": "value_158_"},
              {"first": "key_191_", "second": "value_191_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 564425
          }
        },
        {
          "key": 13,
          "value": {
            "values": [
              {"first": "key_182_", "second": "value_182_"},
              {"first": "key_73_", "second": "value_73_"},
              {"first": "key_59_", "second": "value_59_"},
              {"first": "key_113_", "second": "value_113_"},
              {"first": "key_43_", "second": "value_43_"},
              {"first": "key_161_", "second": "value_161_"},
              {"first": "key_53_", "second": "value_53_"},
              {"first": "key_94_", "second": "value_94_"},
              {"first": "key_57_", "second": "value_57_"},
              {"first": "key_114_", "second": "value_114_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2151301236
          }
        },
        {
          "key": 45,
          "value": {
            "values": [
              {"first": "key_71_", "second": "value_71_"},
              {"first": "key_107_", "second": "value_107_"},
              {"first": "key_90_", "second": "value_90_"},
              {"first": "key_137_", "second": "value_137_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1073774721
          }
        },
        {
          "key": 14,
          "value": {
            "values": [
              {"first": "key_15_", "second": "value_15_"},
              {"first": "key_177_", "second": "value_177_"},
              {"first": "key_172_", "second": "value_172_"},
              {"first": "key_96_", "second": "value_96_"},
              {"first": "key_139_", "second": "value_139_"},
              {"first": "key_132_", "second": "value_132_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 4203153
          }
        },
        {
          "key": 46,
          "value": {
            "values": [
              {"first": "key_159_", "second": "value_159_"},
              {"first": "key_152_", "second": "value_152_"},
              {"first": "key_67_", "second": "value_67_"},
              {"first": "key_23_", "second": "value_23_"},
              {"first": "key_198_", "second": "value_198_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 344208
          }
        }
      ],
      "collisions": []
    },
    "containers": [{"key": 0, "value": 0}]
  }
})";
    const auto expected      = nlohmann::json::parse(expected_json);
    const auto actual        = nlohmann::json::parse(ar_str);
    REQUIRE(expected == actual);

    const auto loaded_archive = test::from_json<
        immer_archive::champ::container_archive_load<Container>>(ar_str);
    REQUIRE(loaded_archive.containers.size() == 1);

    auto loader       = immer_archive::champ::container_loader{loaded_archive};
    const auto loaded = loader.load(set_id);
    REQUIRE(loaded == set);
}

TEST_CASE("Test loading a big map with xxHash")
{
    using Container = immer::
        map<std::string, std::string, immer_archive::xx_hash<std::string>>;

    const auto set           = gen_map(Container{}, 200);
    const auto [ar, set_id]  = immer_archive::champ::save_to_archive(set, {});
    const auto ar_str        = test::to_json(ar);
    const auto expected_json = R"({
  "value0": {
    "nodes": {
      "inners": [
        {
          "key": 20,
          "value": {
            "values": [
              {"first": "key_5_", "second": "value_5_"},
              {"first": "key_171_", "second": "value_171_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2164260864
          }
        },
        {
          "key": 21,
          "value": {
            "values": [
              {"first": "key_23_", "second": "value_23_"},
              {"first": "key_97_", "second": "value_97_"},
              {"first": "key_125_", "second": "value_125_"},
              {"first": "key_182_", "second": "value_182_"},
              {"first": "key_55_", "second": "value_55_"},
              {"first": "key_121_", "second": "value_121_"},
              {"first": "key_34_", "second": "value_34_"}
            ],
            "children": [22],
            "nodemap": 65536,
            "datamap": 1413515268
          }
        },
        {
          "key": 22,
          "value": {
            "values": [
              {"first": "key_170_", "second": "value_170_"},
              {"first": "key_73_", "second": "value_73_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 536887296
          }
        },
        {
          "key": 23,
          "value": {
            "values": [
              {"first": "key_198_", "second": "value_198_"},
              {"first": "key_197_", "second": "value_197_"},
              {"first": "key_172_", "second": "value_172_"},
              {"first": "key_140_", "second": "value_140_"},
              {"first": "key_162_", "second": "value_162_"}
            ],
            "children": [24, 25],
            "nodemap": 67108872,
            "datamap": 17039427
          }
        },
        {
          "key": 24,
          "value": {
            "values": [
              {"first": "key_32_", "second": "value_32_"},
              {"first": "key_111_", "second": "value_111_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33554688
          }
        },
        {
          "key": 25,
          "value": {
            "values": [],
            "children": [26],
            "nodemap": 1048576,
            "datamap": 0
          }
        },
        {
          "key": 26,
          "value": {
            "values": [
              {"first": "key_127_", "second": "value_127_"},
              {"first": "key_181_", "second": "value_181_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 131074
          }
        },
        {
          "key": 27,
          "value": {
            "values": [
              {"first": "key_100_", "second": "value_100_"},
              {"first": "key_149_", "second": "value_149_"},
              {"first": "key_94_", "second": "value_94_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 268501000
          }
        },
        {
          "key": 28,
          "value": {
            "values": [
              {"first": "key_75_", "second": "value_75_"},
              {"first": "key_164_", "second": "value_164_"},
              {"first": "key_37_", "second": "value_37_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 134479874
          }
        },
        {
          "key": 29,
          "value": {
            "values": [
              {"first": "key_28_", "second": "value_28_"},
              {"first": "key_148_", "second": "value_148_"},
              {"first": "key_89_", "second": "value_89_"}
            ],
            "children": [30],
            "nodemap": 67108864,
            "datamap": 1082138624
          }
        },
        {
          "key": 30,
          "value": {
            "values": [
              {"first": "key_143_", "second": "value_143_"},
              {"first": "key_42_", "second": "value_42_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 536871424
          }
        },
        {
          "key": 31,
          "value": {
            "values": [
              {"first": "key_152_", "second": "value_152_"},
              {"first": "key_33_", "second": "value_33_"},
              {"first": "key_67_", "second": "value_67_"},
              {"first": "key_153_", "second": "value_153_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1441800
          }
        },
        {
          "key": 0,
          "value": {
            "values": [],
            "children": [
              1,
              2,
              4,
              5,
              7,
              9,
              11,
              13,
              14,
              16,
              17,
              21,
              23,
              27,
              28,
              29,
              31,
              32,
              33,
              35,
              36,
              38,
              39,
              40,
              43,
              44,
              45,
              46,
              47,
              48,
              50
            ],
            "nodemap": 4294934527,
            "datamap": 0
          }
        },
        {
          "key": 32,
          "value": {
            "values": [
              {"first": "key_102_", "second": "value_102_"},
              {"first": "key_120_", "second": "value_120_"},
              {"first": "key_59_", "second": "value_59_"},
              {"first": "key_51_", "second": "value_51_"},
              {"first": "key_137_", "second": "value_137_"},
              {"first": "key_54_", "second": "value_54_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1881276420
          }
        },
        {
          "key": 1,
          "value": {
            "values": [
              {"first": "key_95_", "second": "value_95_"},
              {"first": "key_29_", "second": "value_29_"},
              {"first": "key_41_", "second": "value_41_"},
              {"first": "key_31_", "second": "value_31_"},
              {"first": "key_53_", "second": "value_53_"},
              {"first": "key_116_", "second": "value_116_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2153783392
          }
        },
        {
          "key": 33,
          "value": {
            "values": [{"first": "key_91_", "second": "value_91_"}],
            "children": [34],
            "nodemap": 32768,
            "datamap": 32
          }
        },
        {
          "key": 2,
          "value": {
            "values": [
              {"first": "key_4_", "second": "value_4_"},
              {"first": "key_0_", "second": "value_0_"},
              {"first": "key_160_", "second": "value_160_"},
              {"first": "key_190_", "second": "value_190_"},
              {"first": "key_98_", "second": "value_98_"},
              {"first": "key_117_", "second": "value_117_"},
              {"first": "key_17_", "second": "value_17_"}
            ],
            "children": [3],
            "nodemap": 16384,
            "datamap": 17957728
          }
        },
        {
          "key": 34,
          "value": {
            "values": [
              {"first": "key_10_", "second": "value_10_"},
              {"first": "key_141_", "second": "value_141_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33554496
          }
        },
        {
          "key": 3,
          "value": {
            "values": [
              {"first": "key_83_", "second": "value_83_"},
              {"first": "key_128_", "second": "value_128_"},
              {"first": "key_66_", "second": "value_66_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 805830656
          }
        },
        {
          "key": 35,
          "value": {
            "values": [
              {"first": "key_77_", "second": "value_77_"},
              {"first": "key_68_", "second": "value_68_"},
              {"first": "key_38_", "second": "value_38_"},
              {"first": "key_156_", "second": "value_156_"},
              {"first": "key_85_", "second": "value_85_"},
              {"first": "key_133_", "second": "value_133_"},
              {"first": "key_61_", "second": "value_61_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 637599906
          }
        },
        {
          "key": 4,
          "value": {
            "values": [
              {"first": "key_155_", "second": "value_155_"},
              {"first": "key_14_", "second": "value_14_"},
              {"first": "key_132_", "second": "value_132_"},
              {"first": "key_86_", "second": "value_86_"},
              {"first": "key_19_", "second": "value_19_"},
              {"first": "key_119_", "second": "value_119_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 203686401
          }
        },
        {
          "key": 36,
          "value": {
            "values": [
              {"first": "key_6_", "second": "value_6_"},
              {"first": "key_3_", "second": "value_3_"},
              {"first": "key_112_", "second": "value_112_"},
              {"first": "key_110_", "second": "value_110_"},
              {"first": "key_62_", "second": "value_62_"},
              {"first": "key_195_", "second": "value_195_"},
              {"first": "key_92_", "second": "value_92_"},
              {"first": "key_107_", "second": "value_107_"},
              {"first": "key_50_", "second": "value_50_"}
            ],
            "children": [37],
            "nodemap": 524288,
            "datamap": 1636838404
          }
        },
        {
          "key": 5,
          "value": {
            "values": [
              {"first": "key_69_", "second": "value_69_"},
              {"first": "key_104_", "second": "value_104_"},
              {"first": "key_193_", "second": "value_193_"},
              {"first": "key_101_", "second": "value_101_"},
              {"first": "key_65_", "second": "value_65_"},
              {"first": "key_189_", "second": "value_189_"}
            ],
            "children": [6],
            "nodemap": 131072,
            "datamap": 557858976
          }
        },
        {
          "key": 37,
          "value": {
            "values": [
              {"first": "key_199_", "second": "value_199_"},
              {"first": "key_58_", "second": "value_58_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 8388736
          }
        },
        {
          "key": 6,
          "value": {
            "values": [
              {"first": "key_167_", "second": "value_167_"},
              {"first": "key_145_", "second": "value_145_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 320
          }
        },
        {
          "key": 38,
          "value": {
            "values": [
              {"first": "key_70_", "second": "value_70_"},
              {"first": "key_191_", "second": "value_191_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2105344
          }
        },
        {
          "key": 7,
          "value": {
            "values": [
              {"first": "key_44_", "second": "value_44_"},
              {"first": "key_184_", "second": "value_184_"},
              {"first": "key_72_", "second": "value_72_"},
              {"first": "key_2_", "second": "value_2_"},
              {"first": "key_27_", "second": "value_27_"},
              {"first": "key_196_", "second": "value_196_"},
              {"first": "key_179_", "second": "value_179_"}
            ],
            "children": [8],
            "nodemap": 16,
            "datamap": 272996
          }
        },
        {
          "key": 39,
          "value": {
            "values": [
              {"first": "key_113_", "second": "value_113_"},
              {"first": "key_21_", "second": "value_21_"},
              {"first": "key_146_", "second": "value_146_"},
              {"first": "key_22_", "second": "value_22_"},
              {"first": "key_16_", "second": "value_16_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 67117201
          }
        },
        {
          "key": 8,
          "value": {
            "values": [
              {"first": "key_118_", "second": "value_118_"},
              {"first": "key_78_", "second": "value_78_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 528
          }
        },
        {
          "key": 40,
          "value": {
            "values": [
              {"first": "key_15_", "second": "value_15_"},
              {"first": "key_108_", "second": "value_108_"},
              {"first": "key_84_", "second": "value_84_"}
            ],
            "children": [41, 42],
            "nodemap": 2097184,
            "datamap": 12582920
          }
        },
        {
          "key": 9,
          "value": {
            "values": [
              {"first": "key_105_", "second": "value_105_"},
              {"first": "key_158_", "second": "value_158_"},
              {"first": "key_168_", "second": "value_168_"},
              {"first": "key_87_", "second": "value_87_"},
              {"first": "key_180_", "second": "value_180_"},
              {"first": "key_24_", "second": "value_24_"}
            ],
            "children": [10],
            "nodemap": 16,
            "datamap": 1074930180
          }
        },
        {
          "key": 41,
          "value": {
            "values": [
              {"first": "key_188_", "second": "value_188_"},
              {"first": "key_130_", "second": "value_130_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 276824064
          }
        },
        {
          "key": 10,
          "value": {
            "values": [
              {"first": "key_20_", "second": "value_20_"},
              {"first": "key_174_", "second": "value_174_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33554688
          }
        },
        {
          "key": 42,
          "value": {
            "values": [
              {"first": "key_64_", "second": "value_64_"},
              {"first": "key_165_", "second": "value_165_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 1081344
          }
        },
        {
          "key": 11,
          "value": {
            "values": [
              {"first": "key_131_", "second": "value_131_"},
              {"first": "key_114_", "second": "value_114_"},
              {"first": "key_8_", "second": "value_8_"},
              {"first": "key_18_", "second": "value_18_"},
              {"first": "key_134_", "second": "value_134_"},
              {"first": "key_7_", "second": "value_7_"},
              {"first": "key_135_", "second": "value_135_"},
              {"first": "key_46_", "second": "value_46_"},
              {"first": "key_150_", "second": "value_150_"}
            ],
            "children": [12],
            "nodemap": 4096,
            "datamap": 92852
          }
        },
        {
          "key": 43,
          "value": {
            "values": [
              {"first": "key_139_", "second": "value_139_"},
              {"first": "key_96_", "second": "value_96_"},
              {"first": "key_43_", "second": "value_43_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2147483968
          }
        },
        {
          "key": 12,
          "value": {
            "values": [
              {"first": "key_185_", "second": "value_185_"},
              {"first": "key_99_", "second": "value_99_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2101248
          }
        },
        {
          "key": 44,
          "value": {
            "values": [
              {"first": "key_178_", "second": "value_178_"},
              {"first": "key_106_", "second": "value_106_"},
              {"first": "key_88_", "second": "value_88_"},
              {"first": "key_144_", "second": "value_144_"},
              {"first": "key_138_", "second": "value_138_"},
              {"first": "key_124_", "second": "value_124_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 3759145488
          }
        },
        {
          "key": 13,
          "value": {
            "values": [
              {"first": "key_13_", "second": "value_13_"},
              {"first": "key_154_", "second": "value_154_"},
              {"first": "key_151_", "second": "value_151_"},
              {"first": "key_93_", "second": "value_93_"},
              {"first": "key_175_", "second": "value_175_"},
              {"first": "key_9_", "second": "value_9_"},
              {"first": "key_74_", "second": "value_74_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 18057224
          }
        },
        {
          "key": 45,
          "value": {
            "values": [
              {"first": "key_40_", "second": "value_40_"},
              {"first": "key_192_", "second": "value_192_"},
              {"first": "key_56_", "second": "value_56_"},
              {"first": "key_187_", "second": "value_187_"},
              {"first": "key_35_", "second": "value_35_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 16795392
          }
        },
        {
          "key": 14,
          "value": {
            "values": [
              {"first": "key_25_", "second": "value_25_"},
              {"first": "key_52_", "second": "value_52_"},
              {"first": "key_26_", "second": "value_26_"},
              {"first": "key_166_", "second": "value_166_"},
              {"first": "key_109_", "second": "value_109_"},
              {"first": "key_81_", "second": "value_81_"}
            ],
            "children": [15],
            "nodemap": 512,
            "datamap": 2751864832
          }
        },
        {
          "key": 46,
          "value": {
            "values": [
              {"first": "key_159_", "second": "value_159_"},
              {"first": "key_71_", "second": "value_71_"},
              {"first": "key_161_", "second": "value_161_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 2701131776
          }
        },
        {
          "key": 15,
          "value": {
            "values": [
              {"first": "key_103_", "second": "value_103_"},
              {"first": "key_115_", "second": "value_115_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 262148
          }
        },
        {
          "key": 47,
          "value": {
            "values": [
              {"first": "key_12_", "second": "value_12_"},
              {"first": "key_183_", "second": "value_183_"},
              {"first": "key_82_", "second": "value_82_"},
              {"first": "key_49_", "second": "value_49_"},
              {"first": "key_80_", "second": "value_80_"},
              {"first": "key_45_", "second": "value_45_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 136478768
          }
        },
        {
          "key": 16,
          "value": {
            "values": [
              {"first": "key_39_", "second": "value_39_"},
              {"first": "key_177_", "second": "value_177_"},
              {"first": "key_30_", "second": "value_30_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 68157696
          }
        },
        {
          "key": 48,
          "value": {
            "values": [
              {"first": "key_186_", "second": "value_186_"},
              {"first": "key_90_", "second": "value_90_"},
              {"first": "key_122_", "second": "value_122_"},
              {"first": "key_176_", "second": "value_176_"},
              {"first": "key_129_", "second": "value_129_"},
              {"first": "key_142_", "second": "value_142_"},
              {"first": "key_123_", "second": "value_123_"},
              {"first": "key_11_", "second": "value_11_"},
              {"first": "key_63_", "second": "value_63_"},
              {"first": "key_163_", "second": "value_163_"}
            ],
            "children": [49],
            "nodemap": 64,
            "datamap": 1092362778
          }
        },
        {
          "key": 17,
          "value": {
            "values": [
              {"first": "key_136_", "second": "value_136_"},
              {"first": "key_126_", "second": "value_126_"},
              {"first": "key_76_", "second": "value_76_"},
              {"first": "key_169_", "second": "value_169_"},
              {"first": "key_1_", "second": "value_1_"}
            ],
            "children": [18, 20],
            "nodemap": 96,
            "datamap": 67191304
          }
        },
        {
          "key": 49,
          "value": {
            "values": [
              {"first": "key_36_", "second": "value_36_"},
              {"first": "key_79_", "second": "value_79_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 268468224
          }
        },
        {
          "key": 18,
          "value": {
            "values": [],
            "children": [19],
            "nodemap": 33554432,
            "datamap": 0
          }
        },
        {
          "key": 50,
          "value": {
            "values": [
              {"first": "key_60_", "second": "value_60_"},
              {"first": "key_157_", "second": "value_157_"},
              {"first": "key_47_", "second": "value_47_"},
              {"first": "key_173_", "second": "value_173_"}
            ],
            "children": [51],
            "nodemap": 64,
            "datamap": 268962048
          }
        },
        {
          "key": 19,
          "value": {
            "values": [
              {"first": "key_57_", "second": "value_57_"},
              {"first": "key_48_", "second": "value_48_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 3221225472
          }
        },
        {
          "key": 51,
          "value": {
            "values": [
              {"first": "key_194_", "second": "value_194_"},
              {"first": "key_147_", "second": "value_147_"}
            ],
            "children": [],
            "nodemap": 0,
            "datamap": 33562624
          }
        }
      ],
      "collisions": []
    },
    "containers": [{"key": 0, "value": 0}]
  }
})";
    // REQUIRE(ar_str == "");
    const auto expected = nlohmann::json::parse(expected_json);
    const auto actual   = nlohmann::json::parse(ar_str);
    REQUIRE(expected == actual);

    const auto loaded_archive = test::from_json<
        immer_archive::champ::container_archive_load<Container>>(ar_str);
    REQUIRE(loaded_archive.containers.size() == 1);

    auto loader       = immer_archive::champ::container_loader{loaded_archive};
    const auto loaded = loader.load(set_id);
    REQUIRE(loaded == set);
}
