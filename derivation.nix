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
  xxHash,
  lib,
  build-tests ? false,
}:
stdenv.mkDerivation rec {
  pname = "immer-archive";
  version = "0.0";
  src = lib.cleanSourceWith {
    filter = name: type: !(lib.hasSuffix ".nix" name);
    src = ./.;
  };
  buildInputs = [
    spdlog
    arximboldi-cereal
    fmt_9
    catch2_3
    boost
    nlohmann_json
    immer
    xxHash
  ];

  doCheck = true;
  hardeningDisable = ["all"];
  dontStrip = true;

  # Currently, the code has a memory leak
  cmakeFlags = [
    "-DTESTS_WITH_LEAK_SANITIZER=OFF"
    "-DBUILD_TESTS=${
      if build-tests
      then "ON"
      else "OFF"
    }"
  ];

  nativeBuildInputs = [ninja cmake];
}
