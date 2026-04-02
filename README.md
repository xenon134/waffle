# Waffle Imager
A high-performance, lightweight Qt image viewer designed for speed and smooth interaction with large images.

## Features
- **Dynamic Zooming**: Supports "Fit to Window", "1:1 Pixel-Perfect", and "Custom" zoom modes.
- **Smooth Navigation**: Pan by dragging the image with the left mouse button.
- **Zoom to Cursor**: Zoom in and out using the mouse wheel, anchored to your pointer's position.
- **Multi-format Support**: Native support for common formats (JPG, PNG, BMP, etc.) and extended support for HEIC, AVIF, JP2, and more via ImageMagick.
- **Quick Actions**: Integrated buttons for copying to clipboard, reloading, opening in MSPaint, and a custom delete utility.
- **Smart Proxying**: Automatically converts heavier formats to temporary PNGs and caches them by hash for instant re-loading.
- **RGBA Support**: Automatically composites transparent images onto a clean white background.

## Usage

### Command Line
Launch the application by passing files or a directory:
```sh
# Open a single image
waffle.exe image.png

# Open multiple images
waffle.exe img1.jpg img2.png img3.webp

# Open a directory (scans for all supported images)
waffle.exe C:\Photos\Summer
```

### Navigation
- **Left/Right Arrows**: Cycle through images in the list.
- **Mouse Wheel**: Zoom in/out at the cursor position.
- **Left Click + Drag**: Pan around zoomed-in images.
- **Toolbar Buttons**:
    - `← / →`: Previous/Next image.
    - `⛶`: Fit image to window.
    - `1:1`: View image at original size.
    - `⎘`: Copy current image path to clipboard (via `copyimage.exe`).
    - `↺`: Reload original image.
    - `✎`: Open current image in MSPaint.
    - `X`: Run custom delete utility.

### Configuration
The application looks for a `config.ini` in the same directory as the executable, or at the path specified by the `WaffleImager_config` environment variable.

**Example `config.ini`:**
```ini
magickPath=C:\Program Files\ImageMagick-7.1.1-Q16-HDRI\magick.exe
```

<details>
<summary>Installation & Build Details</summary>

#### Requirements

- CMake 3.16+
- Qt 5 or Qt 6 (Widgets module)
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

Running

```sh
.\build\Debug\waffle.exe
```

</details>
