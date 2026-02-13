# ScrenKey

A tiny, transparent on-screen key overlay for Windows. Shows the key you just pressed in a floating block — useful for screencasts, tutorials, and live demos.

Single C file. No dependencies beyond the Windows SDK.

![Windows](https://img.shields.io/badge/platform-Windows-blue)
![C](https://img.shields.io/badge/language-C-grey)
![License](https://img.shields.io/badge/license-MIT-green)

## Features

- Transparent always-on-top overlay — only the key block is visible
- Displays letters, numbers, punctuation, F-keys, arrows, and modifier keys
- Draggable — left-click and drag to reposition
- Right-click to close
- Double-buffered GDI rendering (no flicker)
- Single file, ~250 lines of C

## Build

**GCC (MinGW):**

```sh
gcc screnkey.c -o screnkey.exe -lgdi32 -luser32 -mwindows -municode
```

**MSVC:**

```sh
cl screnkey.c /link user32.lib gdi32.lib
```

## Usage

```sh
screnkey.exe 
```
note : press any key to popup the window, it won't appear untill you press any key.

## Screenshot
<img width="439" height="466" alt="miniScreenKey" src="https://github.com/user-attachments/assets/b5d061b9-e034-4947-b237-9424a0ad1df2" />



| Action | Effect |
|---|---|
| Press any key | Displays the key in the overlay |
| Left-click + drag | Move the window |
| Right-click | Close |

## Key Labels

| Key | Label |
|---|---|
| A-Z, 0-9 | Shown as-is |
| Space | `SP` |
| Enter | `ENT` |
| Backspace | `BS` |
| Tab | `TAB` |
| Escape | `ESC` |
| Shift / Ctrl / Alt | `SHF` / `CTL` / `ALT` |
| Arrow keys | Unicode arrows |
| F1-F24 | `F1`-`F24` |

## Configuration

Edit the `#define` constants at the top of `screnkey.c`:

```c
#define WIN_SIZE      120          // window dimensions (px)
#define FONT_SIZE     32           // key label font size
#define BLOCK_COLOR   RGB(34,34,34)  // background of the key block
#define TEXT_COLOR    RGB(255,255,255) // label color
```

Recompile after changing.

## How It Works

1. Creates a `WS_EX_LAYERED` popup window with a color-key transparency (`SetLayeredWindowAttributes`)
2. Installs a low-level keyboard hook (`WH_KEYBOARD_LL`) to capture all keypresses globally
3. Maps virtual key codes to short display labels
4. Paints a rounded rectangle with the label using GDI, double-buffered to prevent flicker

## License

MIT
