{
  description = "Devshell with VSCodium FHS (Wiki compliant)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    
  };

  outputs = { self, nixpkgs, flake-utils,  ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true; # Required for MS C++ extension binaries if you install them
        };

        # 1. Configure VSCodium FHS according to the Wiki.
        # We add the compilers (gcc, python) INTO the editor's container so the extensions can find them.
        codium-fhs = pkgs.vscodium.fhsWithPackages (ps: [
          ps.gcc          # C Compiler for the extension
          ps.gdb          # Debugger for the extension
          ps.python3      # Python runtime
          ps.cmake        # Build tools
          
          # Add the antigravity package inside the editor's environment too, 
        ]);

        codium=pkgs.vscode-with-extensions.override {
        vscode = codium-fhs;
        vscodeExtensions = with pkgs.vscode-extensions; [
          ms-vscode.cpptools
          ms-vscode.cmake-tools
          platformio.platformio-vscode-ide
        ] ++ pkgs.vscode-utils.extensionsFromVscodeMarketplace [
      ];
   };
in {
        devShells.default = pkgs.mkShell {
          # 2. Packages available in your terminal
          buildInputs = [
            codium
            pkgs.gcc
            pkgs.python3
          ];

          shellHook = ''
            echo "------------------------------------------------------------------"
            echo "VSCodium FHS Loaded."
            echo ""
            echo "------------------------------------------------------------------"
          '';
        };
      }
    );
}
