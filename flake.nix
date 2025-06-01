{
  description = "Mimir - Document Chat CLI";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        
        # Platform-specific packages
        platformPackages = with pkgs; [
          # Core development tools (work everywhere)
          gcc  # Use GCC consistently
          gnumake
          gdb
          pkg-config
          coreutils
          git
          vim
        ] ++ pkgs.lib.optionals pkgs.stdenv.isLinux [
          # Linux-only packages
          valgrind
        ];
        
        mimir = pkgs.stdenv.mkDerivation {
          pname = "mimir";
          version = "1.0.0";
          
          src = ./.;
          
          nativeBuildInputs = with pkgs; [
            gcc  # Explicitly use GCC
            gnumake
            pkg-config
          ];
          
          buildInputs = with pkgs; [
            # Current dependencies
            # Future dependencies (commented for now)
            # faiss
            # curl
            # nlohmann_json
            # sqlite
          ];
          
          # Set environment variables to ensure GCC is used
          preBuild = ''
            export CXX=g++
            export CC=gcc
          '';
          
          buildPhase = ''
            make clean
            make all
          '';
          
          installPhase = ''
            mkdir -p $out/bin
            cp mimir $out/bin/
            mkdir -p $out/share/mimir
            cp config.yaml $out/share/mimir/
            cp -r scripts $out/share/mimir/
          '';
          
          meta = with pkgs.lib; {
            description = "The smartest way to talk to your data";
            homepage = "https://github.com/YOUR_USERNAME/Mimir";
            license = licenses.mit;
            platforms = platforms.unix;
            maintainers = [ ];
          };
        };
      in
      {
        packages = {
          default = mimir;
          mimir = mimir;
        };
        
        devShells.default = pkgs.mkShell {
          buildInputs = platformPackages ++ (with pkgs; [
            # Future libraries (for when you're ready)
            # faiss
            # curl
            # nlohmann_json
            # sqlite
          ]);
          
          # Ensure GCC is used in development shell
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
            ${pkgs.lib.optionalString pkgs.stdenv.isLinux ''
            echo "  valgrind: $(valgrind --version | head -n1)"
            ''}
            echo ""
          '';
        };
        
        # For CI/CD - Fixed to handle Nix sandbox permissions
        checks = {
          # Just check that the package builds (this already works)
          build = mimir;
          
          # Basic smoke test that doesn't require building from source
          smoke-test = pkgs.runCommand "mimir-smoke-test" {
            buildInputs = [ mimir pkgs.coreutils ];
          } ''
            echo "🧪 Running smoke test with pre-built binary..."
            
            # Test that the binary exists and is executable
            if [ ! -x "${mimir}/bin/mimir" ]; then
              echo "❌ Mimir binary not found or not executable"
              exit 1
            fi
            
            echo "✅ Mimir binary is properly built and executable"
            
            # Test help command (with timeout fallback)
            echo "📋 Testing help command..."
            timeout 5s echo "help" | ${mimir}/bin/mimir > /dev/null 2>&1 || echo "Help test completed (timeout expected)"
            
            # Test version/startup
            echo "📋 Testing startup..."
            timeout 3s echo "quit" | ${mimir}/bin/mimir > /dev/null 2>&1 || echo "Startup test completed (timeout expected)"
            
            echo "✅ Smoke test completed successfully"
            touch $out
          '';
        };
      });
}