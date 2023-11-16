{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    google-fuzztest = {
      url = "github:google/fuzztest";
      flake = false;
    };
    google-fuzztest-absl = {
      url = "github:abseil/abseil-cpp/db08109eeb15fcd856761557f1668c2b34690036";
      flake = false;
    };
    google-fuzztest-gtest = {
      url = "github:google/googletest/v1.14.0";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
    google-fuzztest,
    google-fuzztest-absl,
    google-fuzztest-gtest,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      clang-format = pkgs.runCommand "clang-format" {} ''
        mkdir -p $out/bin
        ln -s ${pkgs.llvmPackages_16.clang-unwrapped}/bin/clang-format $out/bin/
      '';
      our_llvm = pkgs.llvmPackages_14;
      google-fuzztest-antlr = pkgs.stdenv.mkDerivation rec {
        pname = "antlr4-cpp-runtime";
        version = "4.12.0";
        src = pkgs.fetchurl {
          url = "https://www.antlr.org/download/${pname}-${version}-source.zip";
          hash = "sha256-ZC1ZhU3cDOu1sjsiM60KhyPu8g5m73i1uJjQpnVWiTs=";
        };
        nativeBuildInputs = with pkgs; [unzip];
        unpackPhase = ''
          mkdir -p $out
          cp $src $out/
          cd $out
          unzip *.zip
          rm *.zip
        '';
      };
    in rec {
      # devShell = pkgs.mkShell.override {stdenv = our_llvm.stdenv;} {
      #   packages = with pkgs; [
      #     clang-format
      #     cmake-format
      #     cmake
      #     ninja
      #     spdlog
      #     cereal
      #     fmt_9
      #     catch2_3
      #     boost

      #     # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
      #     our_llvm.bintools-unwrapped
      #   ];

      #   shellHook = ''
      #     export GOOGLE_FUZZTEST=${google-fuzztest}
      #   '';
      # };

      defaultPackage = our_llvm.stdenv.mkDerivation rec {
        pname = "immer-archive";
        version = "0.0";
        src = ./immer-test;
        buildInputs = with pkgs; [
          spdlog
          cereal
          fmt_9
          catch2_3
          boost
          nlohmann_json
          re2
        ];
        GOOGLE_FUZZTEST = google-fuzztest;
        ABSEIL_CPP = google-fuzztest-absl;
        GOOGLE_FUZZTEST_GTEST = google-fuzztest-gtest;
        GOOGLE_FUZZTEST_ANTLR = google-fuzztest-antlr;
        nativeBuildInputs = with pkgs; [ninja cmake];
        cmakeFlags = ["-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${src}/cmake/provider.cmake"];
      };
    });
}
