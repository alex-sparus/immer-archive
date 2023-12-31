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
    immer
  ];

  doCheck = true;

  # Currently, the code has a memory leak
  cmakeFlags = ["-DTESTS_WITH_LEAK_SANITIZER=OFF" "-DBUILD_TESTS=OFF"];

  nativeBuildInputs = [ninja cmake];
}
