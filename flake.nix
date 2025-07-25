{
  description = "Mimir dev environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };

        # Custom CPR package (since not in upstream Nixpkgs)
        cpr = pkgs.stdenv.mkDerivation rec {
          pname = "cpr";
          version = "1.10.4";
          src = pkgs.fetchFromGitHub {
            owner = "libcpr";
            repo = "cpr";
            rev = "${version}";
            sha256 = "sha256-8qRNlZgBB71t/FSFPnxFhr02OuD2erLVeoc6wAx3LKk=";
          };
          nativeBuildInputs = [ pkgs.cmake pkgs.pkg-config pkgs.git ];
          buildInputs = [ pkgs.openssl pkgs.curl pkgs.zlib ];
          cmakeFlags = [
            "-DCPR_BUILD_TESTS=OFF"
            "-DCPR_BUILD_EXAMPLES=OFF"
            "-DCPR_FORCE_USE_SYSTEM_CURL=ON"
            "-DCPR_FORCE_USE_SYSTEM_ZLIB=ON"
            "-DCPR_FORCE_USE_SYSTEM_OPENSSL=ON"
            "-DBUILD_SHARED_LIBS=ON"
            "-DCPR_USE_SYSTEM_FILESYSTEM=ON"
            "-DCPR_FORCE_USE_SYSTEM_FILESYSTEM=ON"
            "-DCPR_CXX_STANDARD=17"
          ];
          patches = [ ./cpr-no-stdc++fs.patch ];
          preBuild = ''
            # Remove -lstdc++fs from all files in the build directory before building
            find . -type f -exec sed -i.bak 's/-lstdc++fs//g' {} +
          '';
          postPatch = ''
            # Remove -lstdc++fs from all CMake files (fixes macOS/modern clang)
            find . -type f -name '*.cmake' -exec sed -i.bak 's/-lstdc++fs//g' {} +
            find . -type f -name 'CMakeLists.txt' -exec sed -i.bak 's/-lstdc++fs//g' {} +
          '';
          installPhase = ''
            mkdir -p $out/lib $out/include
            cp -r include/cpr $out/include/
            cp build/libcpr* $out/lib/
          '';
        };
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            pkgs.python311
            pkgs.python311.pkgs.pip
            pkgs.gcc
            pkgs.gnumake
            pkgs.bash
            pkgs.gpp
            pkgs.poppler_utils # for pdftotext
            pkgs.tesseract
            pkgs.nlohmann_json
            # cpr removed; use Homebrew for CPR
            # add more tools as needed
          ];
          shellHook = ''
            export CXX=g++
            export CC=gcc
            echo "🚀 Welcome to Mimir development environment!"
            echo "Platform: ${system}"
            echo ""
            echo "Available commands:"
            echo "  make        - Build the project"
            echo "  make run    - Build and run"
            echo "  make clean  - Clean build files"
            echo ""
            echo "Tools available:"
            echo "  gcc: $(gcc --version | head -n1)"
            echo "  make: $(make --version | head -n1)"
            if [ "$(uname)" = "Linux" ]; then
              echo "  valgrind: $(valgrind --version | head -n1)"
            fi
            echo ""
            echo "📄 PDF Tools available:"
            echo "  pdftotext: $(pdftotext -v 2>&1 | head -n1 || echo 'Available')"
            echo "  tesseract: $(tesseract --version 2>&1 | head -n1 || echo 'Available')"
            echo ""
            if [ ! -d venv ]; then
              python -m venv venv
              echo "Created venv. Run 'source venv/bin/activate' and 'pip install -r requirements.txt'"
            fi
            echo "Run: source venv/bin/activate"
          '';
        };
      }
    );
}