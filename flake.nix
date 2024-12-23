{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            autoconf
            automake
            autogen
            libtool
            pkg-config
            glib
            rofi-unwrapped
            cairo
            bear
          ];
        };

        packages = {
          rofi-ts = pkgs.callPackage ./nix { };
          default = self.packages.${system}.rofi-ts;
        };
      });
}
