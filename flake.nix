{
  description = "Freshtrix — experimental Larptrix desktop client port";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              git
              cmake
              ninja
              pkg-config
              gcc
              clang
              python3
              perl
              gnumake
              gdb
              ccache
              openssl
              zlib
              glib
              glibmm
              gtk3
              dbus
              fontconfig
              freetype
              libxkbcommon
              xorg.libX11
              xorg.libXext
              xorg.libXrender
              xorg.libXrandr
              xorg.libXfixes
              xorg.libXi
              xorg.libXcursor
              xorg.libXinerama
              xorg.libXScrnSaver
              xorg.libxcb
              wayland
              libGL
              alsa-lib
              pulseaudio
              pipewire
              libpulseaudio
              libsecret
              hunspell
              minizip
              ffmpeg
              openal
              libopus
              libvpx
              libwebp
              libjpeg
              libpng
              libtiff
              libxml2
              libxslt
              icu
              libevent
              lz4
              xxHash
              libunwind
              breakpad
            ];

            shellHook = ''
              echo "Freshtrix development shell"
              echo "Source branch: larptrix-port"
              echo "This shell provides common Linux build dependencies."
              echo "The inherited FreshGram build still uses Telegram/MTProto and needs a separate port."
            '';
          };
        });

      apps = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          launcher = pkgs.writeShellApplication {
            name = "freshtrix";
            runtimeInputs = [ pkgs.coreutils ];
            text = ''
              candidates=(
                "$PWD/out/Telegram"
                "$PWD/out/Telegram/Telegram"
                "$PWD/out/install/usr/bin/Telegram"
                "$PWD/out/install/usr/local/bin/Telegram"
              )

              for candidate in "${candidates[@]}"; do
                if [ -x "$candidate" ]; then
                  exec "$candidate" "$@"
                fi
              done

              cat >&2 <<'MESSAGE'
              Freshtrix has not been built in this checkout yet.

              1. Enter the build environment: nix develop
              2. Initialize submodules: git submodule update --init --recursive
              3. Follow docs/build-baseline.md for the current upstream build.

              Note: the current source is still mostly FreshGram/Telegram Desktop;
              this launcher does not make it a working Larptrix client by itself.
              MESSAGE
              exit 1
            '';
          };
        in
        {
          default = {
            type = "app";
            program = "${launcher}/bin/freshtrix";
          };
          run = self.apps.${system}.default;
        });
    };
}
