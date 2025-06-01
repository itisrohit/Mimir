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
        
        # For CI/CD - Simplified and more reliable
        checks = {
          # Just check that the package builds
          build = mimir;
          
          # Simplified test that just verifies the build works
          build-test = pkgs.runCommand "mimir-build-test" {
            buildInputs = with pkgs; [ 
              gcc 
              gnumake 
              pkg-config 
              coreutils 
              bash
            ];
            src = ./.;
          } ''
            # Copy source for testing
            cp -r $src source
            cd source
            
            # Set compiler environment
            export CXX=${pkgs.gcc}/bin/g++
            export CC=${pkgs.gcc}/bin/gcc
            export PATH=${pkgs.gcc}/bin:${pkgs.gnumake}/bin:${pkgs.coreutils}/bin:$PATH
            
            # Verify tools are available by checking if they exist
            echo "🔧 Verifying build tools..."
            if [ ! -x "${pkgs.gcc}/bin/gcc" ]; then
              echo "❌ GCC not found at expected path"
              exit 1
            fi
            
            if [ ! -x "${pkgs.gnumake}/bin/make" ]; then
              echo "❌ Make not found at expected path"
              exit 1
            fi
            
            echo "✅ GCC found: $(${pkgs.gcc}/bin/gcc --version | head -n1)"
            echo "✅ Make found: $(${pkgs.gnumake}/bin/make --version | head -n1)"
            
            # Build the project
            echo "🔨 Building project..."
            ${pkgs.gnumake}/bin/make clean
            ${pkgs.gnumake}/bin/make all
            
            # Verify binary was created
            if [ ! -f "./mimir" ]; then
              echo "❌ Binary not created"
              exit 1
            fi
            
            echo "✅ Binary created successfully"
            echo "✅ Build test completed successfully"
            touch $out
          '';
        };
      });
}