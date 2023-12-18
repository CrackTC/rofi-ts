# rofi-ts

Yet another translate-shell plugin for rofi. Borrowed quite a bit of code from [rofi-calc](https://github.com/svenstaro/rofi-calc)

![demo](img/demo.gif)


## Usage

```bash
rofi -show ts -modi ts
```

## Advanced Usage

```bash
rofi -show ts -modi ts -ts-command "notify-send 'rofi-ts' '{result}'"
```

This will show a notification with the translation result.

![demo-notify](img/demo-notify.png)

