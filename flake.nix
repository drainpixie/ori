{
  inputs = {
    hooks.url = "github:cachix/git-hooks.nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {
    nixpkgs,
    hooks,
    self,
    ...
  }: let
    inherit (nixpkgs) lib;

    systems = [
      "aarch64-linux"
      "i686-linux"
      "x86_64-linux"
      "aarch64-darwin"
      "x86_64-darwin"
    ];

    forAllSystems = lib.genAttrs systems;
  in {
    packages = forAllSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      default = pkgs.stdenv.mkDerivation {
        pname = "ori";
        version = "1.0.0";

        src = ./.;
        nativeBuildInputs = builtins.attrValues {inherit (pkgs) gcc gnumake;};

        installPhase = ''
          runHook preInstall
          mkdir -p $out/bin
          cp ori $out/bin/
          runHook postInstall
        '';
      };
    });

    checks = forAllSystems (system: let
      lib = hooks.lib.${system};
    in {
      pre-commit-check = lib.run {
        src = ./.;
        hooks = {
          alejandra.enable = true;
          convco.enable = true;

          statix = {
            enable = true;
            settings.ignore = ["/.direnv"];
          };
        };
      };
    });

    devShells = forAllSystems (system: let
      pkgs = nixpkgs.legacyPackages.${system};
      check = self.checks.${system}.pre-commit-check;
    in {
      default = pkgs.mkShell {
        buildInputs =
          check.enabledPackages
          ++ builtins.attrValues {
            inherit (pkgs) gcc gdb gnumake valgrind clang-tools bear;
          };

        shellHook =
          check.shellHook
          + ''
            echo "🚀 ori development ready"
          '';
      };
    });

    formatter = forAllSystems (system: nixpkgs.legacyPackages.${system}.alejandra);
  };
}
