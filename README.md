# Waffle Imager
A high-performance, lightweight Qt image viewer designed for speed and smooth interaction with large images.

## Features
- **Dynamic Zooming**: Supports "Fit to Window", "1:1 Pixel-Perfect", and "Custom" zoom modes.
- **Smooth Navigation**: Pan by dragging the image with the left mouse button.
- **Zoom to Cursor**: Zoom in and out using the mouse wheel, anchored to your pointer's position.
- **Multi-format Support**: Native support for common formats (JPG, PNG, BMP, etc.) and extended support for HEIC, AVIF, JP2, and more via ImageMagick.
- **Quick Actions**: Integrated buttons for copying to clipboard, reloading, opening the image in the system editor, and deleting to the recycle bin/trash.
- **Smart Proxying**: Automatically converts heavier formats to temporary PNGs and caches them by hash for instant re-loading.
- **RGBA Support**: Automatically composites transparent images onto a clean white background.

## Usage

### Command Line
Launch the application by passing files or a directory:
```sh
# Open a single image
waffle image.png

# Open multiple images
waffle img1.jpg img2.png img3.webp

# Open a directory (scans for all supported images)
waffle ~/Pictures/Summer
```
On Windows the executable is `waffle.exe`; paths like `C:\Photos\Summer` work there.

### Navigation
- **Left/Right Arrows**: Cycle through images in the list.
- **Mouse Wheel**: Zoom in/out at the cursor position.
- **Left Click + Drag**: Pan around zoomed-in images.
- **Toolbar Buttons**:
    - `← / →`: Previous/Next image.
    - `⛶`: Fit image to window.
    - `1:1`: View image at original size.
    - `⎘`: Copy current image to clipboard.
    - `↺`: Reload original image.
    - `✎`: Open current image in the system editor (MSPaint on Windows, `xdg-open` or `editorCommand` on Linux).
    - `X`: Delete the current image (custom command on Windows, trash on Linux).

### Configuration
The application looks for a `config.ini` in the same directory as the executable, or at the path specified by the `WaffleImager_config` environment variable.

**Example `config.ini`:**
```ini
; leave a value empty to use the platform default
magickPath=
deleteCommand=
editorCommand=
```

| Key | Purpose | Windows default | Linux default |
| --- | --- | --- | --- |
| `magickPath` | ImageMagick executable used for HEIC/HEIF/AVIF/JP2 → PNG proxy conversion | `C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe` | `magick`, else `convert` from `PATH` |
| `deleteCommand` | Program run by the `X` button / `Delete` key | `delet.exe` | `gio trash`, else `trash-put` |
| `editorCommand` | Program run by the `✎` button / `E` key | `mspaint.exe` | `xdg-open` |

If no trash is available for the current filesystem (common on NTFS/FUSE mounts), Waffle asks before
deleting permanently. Set `deleteCommand` (e.g. `deleteCommand=rm`) to pick your own tool instead.

<details>
<summary>Installation & Build Details</summary>

#### Requirements

- CMake 3.16+
- Qt 6 (Widgets module)
- C++17-capable compiler

#### Installing Qt

Option A — vcpkg (recommended if already installed)

```sh
vcpkg install qtbase:x64-windows
```

Then pass the toolchain file to CMake (see Build section below).

Option B — Qt Online Installer

Download and install from <https://www.qt.io/download>. During installation select
the **MSVC 2019 64-bit** (or later) pre-built component.

#### Build

With vcpkg toolchain

```sh
cmake -B build -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

With a manual Qt installation (Qt Online Installer)

```sh
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\msvc2022_64"
cmake --build build
```

### Linux / Unix

#### Requirements

- CMake 3.16+ and a C++17 compiler (`build-essential`)
- Qt 6 Widgets development package
- Optional: ImageMagick for HEIC/HEIF/AVIF/JP2 support
- Optional: `glib2` (`gio trash`) or `trash-cli` (`trash-put`) for the delete button
- Optional: `xdg-utils` (`xdg-open`) for the edit button

```sh
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev libqt6svg6-dev imagemagick xdg-utils

# Fedora
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtsvg-devel ImageMagick xdg-utils
```

#### Build

```sh
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j"$(nproc)"
```

Running

```sh
cp config.ini.example config.ini   # optional, then edit it
./build-linux/waffle path/to/image.jpg
```

</details>
