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
            # add more tools as needed
          ];
          shellHook = ''
            export CXX=g++
            export CC=gcc
            export CPPFLAGS = -I./include
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