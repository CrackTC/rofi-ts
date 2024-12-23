# rofi-ts

Yet another translate-shell plugin for rofi. Borrowed quite a bit of code from [rofi-calc](https://github.com/svenstaro/rofi-calc)

![demo](img/demo.gif)

## Installation

Currently `rofi-ts` provides flake for nix users.

If you are not using nix as package manager, you can clone this repo and compile the plugin yourself.

### Dependencies


- `rofi-dev` (or `rofi-devel`)
- `translate-shell` (runtime dependency)
- `libtool`

`rofi-ts` uses autotools as build system. If installing from git, the following steps should install it:

```bash
autoreconf -i
mkdir build
cd build/
../configure
make
make install
```

## Usage

```bash
rofi -show ts -modi ts
```

Type the sentence to translate. By default, translate-shell guesses the language to translate from, and translates it to your system locale.

You can prefix your query with language info:

- `fr:en Bonjour` will translate 'Bonjour' from french to english
- `:fr Hello` will translate 'Hello' from your locale to french
- `fr: Nombre` will translate 'Nombre' from french to your locale

## Advanced Usage

```bash
rofi -show ts -modi ts -ts-command "notify-send 'rofi-ts' '{result}'"
```

This will show a notification with the translation result.

![demo-notify](img/demo-notify.png)

