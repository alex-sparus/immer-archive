{
  stdenv,
  ninja,
  cmake,
  spdlog,
  arximboldi-cereal,
  fmt_9,
  catch2_3,
  boost,
  nlohmann_json,
  re2,
  immer,
}:
stdenv.mkDerivation rec {
  pname = "immer-archive";
  version = "0.0";
  src = ./.;
  buildInputs = [
    spdlog
    arximboldi-cereal
    fmt_9
    catch2_3
    boost
    nlohmann_json
    re2
    immer
  ];

  # Problems with having those separate json files. Either fix that or rewrite the tests to not depend on the files.
  doCheck = false;

  # Currently, the code has a memory leak
  cmakeFlags = ["-DTESTS_WITH_LEAK_SANITIZER=OFF" "-DBUILD_TESTS=OFF"];

  nativeBuildInputs = [ninja cmake];
}
