{
  inputs = {
    # the only version that seems to support crossplatform compiling lol
    # nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";
    utils.url = "github:numtide/flake-utils";
    rust-overlay = {
      url = "github:oxalica/rust-overlay";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs =
    {
      self,
      nixpkgs,
      utils,
      rust-overlay,
    }:
    utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs {
          inherit system;
          overlays = [ (import rust-overlay) ];
          crossSystem = {
            config = "x86_64-w64-mingw32";
            libc = "msvcrt";
          };
          allowUnsupportedSystem = true;
        };

        toolchain = (pkgs.pkgsBuildHost.rust-bin.fromRustupToolchainFile ./rust-toolchain.toml);
      in
      {
        formatter = (import nixpkgs { inherit system; }).nixfmt-rfc-style;

        packages = {
          discordrpc =
            let
              rust-bin = rust-overlay.lib.mkRustBin { } pkgs.buildPackages;
            in
            pkgs.rustPlatform.buildRustPackage (final: {
              name = "DiscordRPC";
              version = "13.0.0";

              rustToolchain = rust-bin.fromRustupToolchainFile ./rust-toolchain.toml;
              buildInputs = with pkgs; [
                windows.mingw_w64_headers
                windows.pthreads
              ];

              nativeBuildInputs = [
                final.rustToolchain
                pkgs.autoPatchelfHook
                pkgs.pkg-config
              ];

              src = ./.;

              meta = {
                description = "discord rpc impl for northstar";
                homepage = "https://github.com/R2Northstar/NorthstarDiscordRPC";
                license = pkgs.lib.licenses.unlicense;
                maintainers = [ "cat_or_not" ];
              };

              cargoLock = {
                lockFile = ./Cargo.lock;
              };
            });

          default = self.packages.${system}.discordrpc;
        };

        devShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            windows.mingw_w64_headers
            # windows.mcfgthreads
            windows.pthreads
            toolchain
          ];

          nativeBuildInputs = [
            toolchain
          ];
        };
      }
    );
}
