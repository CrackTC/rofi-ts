{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  outputs = { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = import nixpkgs { inherit system; };
      in {
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

          shellHook = ''
            export NIX_CFLAGS_COMPILE="-isystem ${pkgs.lib.getDev pkgs.cairo}/include/cairo"
            exec fish
          '';
        };
      });
}
