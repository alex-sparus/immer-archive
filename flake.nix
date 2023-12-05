{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    arximboldi-cereal-src = {
      url = "github:arximboldi/cereal";
      flake = false;
    };
    immer = {
      url = "github:alex-sparus/immer/immer-archive";
      inputs = {
        nixpkgs.follows = "nixpkgs";
      };
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
    arximboldi-cereal-src,
    immer,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      clang-format = pkgs.runCommand "clang-format" {} ''
        mkdir -p $out/bin
        ln -s ${pkgs.llvmPackages_16.clang-unwrapped}/bin/clang-format $out/bin/
      '';
      our_llvm = pkgs.llvmPackages_14;

      cereal-derivation = {
        stdenv,
        cmake,
        lib,
      }:
        stdenv.mkDerivation rec {
          name = "cereal-${version}";
          version = "git-${commit}";
          commit = arximboldi-cereal-src.rev;
          src = arximboldi-cereal-src;
          nativeBuildInputs = [cmake];
          cmakeFlags = ["-DJUST_INSTALL_CEREAL=true"];
          meta = {
            homepage = "http://uscilab.github.io/cereal";
            description = "A C++11 library for serialization";
            license = lib.licenses.bsd3;
          };
        };
    in rec {
      devShell = pkgs.mkShell.override {stdenv = our_llvm.stdenv;} {
        packages = with pkgs; [
          clang-format
          cmake-format
          cmake
          ninja
          spdlog
          arximboldi-cereal
          fmt_9
          catch2_3
          boost
          immer.defaultPackage.${system}

          # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
          our_llvm.bintools-unwrapped
        ];
      };

      arximboldi-cereal = pkgs.callPackage cereal-derivation {};

      defaultPackage = pkgs.callPackage ./derivation.nix {
        stdenv = our_llvm.stdenv;
        immer = immer.defaultPackage.${system};
        inherit arximboldi-cereal;
      };
    });
}
