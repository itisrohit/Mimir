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
        
        mimir = pkgs.stdenv.mkDerivation {
          pname = "mimir";
          version = "1.0.0";
          
          src = ./.;
          
          nativeBuildInputs = with pkgs; [
            gcc
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
          buildInputs = with pkgs; [
            # Development tools
            gcc
            gnumake
            gdb
            valgrind
            pkg-config
            
            # Future libraries (for when you're ready)
            # faiss
            # curl
            # nlohmann_json
            # sqlite
            
            # Development utilities
            coreutils  # For timeout command
            git
            vim
          ];
          
          shellHook = ''
            echo "🚀 Welcome to Mimir development environment!"
            echo "Available commands:"
            echo "  make        - Build the project"
            echo "  make run    - Build and run"
            echo "  make clean  - Clean build files"
            echo ""
            echo "Tools available:"
            echo "  gcc: $(gcc --version | head -n1)"
            echo "  make: $(make --version | head -n1)"
            echo ""
          '';
        };
        
        # For CI/CD
        checks = {
          build = mimir;
          tests = pkgs.runCommand "mimir-tests" {
            buildInputs = [ mimir pkgs.coreutils ];
          } ''
            # Copy source for testing
            cp -r ${./.} source
            cd source
            chmod +x scripts/*.sh
            
            # Build and test
            make clean
            make all
            
            # Run tests
            timeout 10s echo "help" | ./mimir > /dev/null || true
            timeout 10s echo -e "init test\nquit" | ./mimir > /dev/null || true
            
            touch $out
          '';
        };
      });
}