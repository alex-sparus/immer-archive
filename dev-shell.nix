{
  mkShell,
  our_llvm,
  clang-format,
  cmake-format,
  just,
  fzf,
  cmake,
  ninja,
  spdlog,
  arximboldi-cereal,
  fmt_9,
  catch2_3,
  boost,
  nlohmann_json,
  immer,
  xxHash,
  lib,
  stdenv,
  valgrind,
  lldb,
  pre-commit-check,
}:
mkShell.override {stdenv = our_llvm.stdenv;} {
  NIX_HARDENING_ENABLE = "";
  packages =
    [
      # Tools
      clang-format
      cmake-format
      just
      fzf
      # for the llvm-symbolizer binary, that allows to show stacks in ASAN and LeakSanitizer.
      our_llvm.bintools-unwrapped

      # Build-time
      cmake
      ninja

      # Dependencies
      spdlog
      arximboldi-cereal
      fmt_9
      catch2_3
      boost
      nlohmann_json
      immer
      xxHash
    ]
    ++ lib.optionals stdenv.isLinux [
      valgrind
      lldb
    ];

  shellHook =
    pre-commit-check.shellHook
    + "\n"
    + ''
      source just.bash
      complete -F _just -o bashdefault -o default j
      alias j=just
    '';
}
