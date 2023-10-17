{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
    flake-compat,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      clang-format = pkgs.runCommand "clang-format" {} ''
        mkdir -p $out/bin
        ln -s ${pkgs.llvmPackages_16.clang-unwrapped}/bin/clang-format $out/bin/
      '';
    in rec {
      devShell = pkgs.mkShell.override {stdenv = pkgs.llvmPackages_14.stdenv;} {
        packages = with pkgs; [clang-format cmake ninja spdlog cereal fmt_9];
      };
    });
}
