{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixpkgs-unstable;
    flake-utils.url = "github:numtide/flake-utils";
    pre-commit-hooks = {
      url = "github:cachix/pre-commit-hooks.nix";
      inputs = {
        flake-compat.follows = "flake-compat";
        flake-utils.follows = "flake-utils";
      };
    };
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
    pre-commit-hooks,
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
      arximboldi-cereal = pkgs.callPackage cereal-derivation {};
    in rec {
      checks = {
        pre-commit-check = pre-commit-hooks.lib.${system}.run {
          src = ./.;
          hooks = {
            cmake-format.enable = true;
            clang-format = {
              enable = true;
              types_or = pkgs.lib.mkForce ["c" "c++"];
            };
            alejandra.enable = true;
          };
        };
      };

      devShells.default = pkgs.mkShell.override {stdenv = our_llvm.stdenv;} {
        NIX_HARDENING_ENABLE = "";
        inputsFrom = [self.packages.${system}.immer-archive];
        packages = with pkgs;
          [
            # Tools
            clang-format
            cmake-format
            just
            fzf
            starship
            # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
            our_llvm.bintools-unwrapped

            # Build-time
            cmake
            ninja
          ]
          ++ lib.optionals stdenv.isLinux [
            valgrind
            lldb
          ];

        shellHook =
          self.checks.${system}.pre-commit-check.shellHook
          + "\n"
          + ''
            source just.bash
            complete -F _just -o bashdefault -o default j
            alias j=just
            eval "$(starship init bash)"
          '';
      };

      packages = let
        immer-archive = pkgs.callPackage ./derivation.nix {
          stdenv = our_llvm.stdenv;
          immer = immer.defaultPackage.${system};
          build-with-sanitizer = false;
          inherit arximboldi-cereal;
        };
        immer-archive-asan = immer-archive.override {
          build-tests = true;
          build-with-sanitizer = true;
        };
        debug-tests =
          (immer-archive.override {build-tests = true;})
          .overrideAttrs (prev: {
            cmakeFlags = prev.cmakeFlags ++ ["-DCMAKE_BUILD_TYPE=Debug"];
            doCheck = false;
          });
      in
        {
          inherit immer-archive debug-tests immer-archive-asan;
          default = immer-archive;

          run-fuzz-flex-vector = pkgs.writeShellApplication {
            name = "runner";
            text = ''
              ${immer-archive-asan}/bin/fuzz/flex-vector
            '';
          };
        }
        // (
          if pkgs.stdenv.isLinux
          then {
            # tag valgrind contains the test that is broken and must be fixed
            run-tests-with-valgrind = pkgs.writeShellApplication {
              name = "runner";
              runtimeInputs = [pkgs.valgrind];
              text = ''
                valgrind --suppressions=${debug-tests}/bin/valgrind.supp ${debug-tests}/bin/tests
              '';
            };

            run-broken-test = pkgs.writeShellApplication {
              name = "runner";
              runtimeInputs = [pkgs.valgrind];
              text = ''
                valgrind ${debug-tests}/bin/tests '[broken]'
              '';
            };
          }
          else {}
        );
    });
}
