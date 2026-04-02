This is the story of how the imager.py application was ported to C++, to address the limitations of Python in terms of performance and UI responsiveness when handling large images.

Client: Make an Image Viewer app that loads image(s), opens and displays it, supports window resizing, always keeping the image as big as possible whithin bounds and zooming in and out as needed to keep it that way.

And thus, it begins. main.cpp was created to implement a custom QLabel subclass called ImageViewer which manages maintaining the original image in memory using a QPixmap. When an image is provided, the viewer uses loadImage() to read the file into memory and initially display it scaled to the size of the viewer. Whenever the window changes size, the viewer re-calculates the scaled size based on the new dimensions, maintaining Qt::KeepAspectRatio.

Client: Make it so that it exists in three different states for zoom behaviour: fit, one and custom. In the fit state, it acts like it does now. In the one state, it is pixel-perfect with the screen i.e. 100% zoom. Scrolling up or down moves the app from the current state to the custom state. 0.1 is added or subtracted from the zoom level on scrolling up and down.

With QVBoxLayout to hold the image at the top and the taskbar at the bottom. Finally, a 50px tall horizontal layout taskbar was docked at the bottom with "Fit" and "One" QPushButtons.

At this point, a bug was discovered: when zooming in, the window itself was resizing to fit the new image dimensions instead of keeping the window size fixed and allowing the image to be zoomed beyond it. This was an unintended consequence of how the QLabel handled large pixmaps within the layout. I fixed it by wrapping the image label in a QScrollArea, which allows the image to be zoomed beyond the window size without resizing the window itself. Now when you zoom in, you can scroll to see the rest of the image, and the window stays fixed at its current size.

The updates were made to hide the scrollbars and ensure scrolling always triggered zooming. Qt::ScrollBarAlwaysOff. Additionally, a global scroll hook was established by setting an eventFilter on the QScrollArea's viewport. This intercepted any wheel events the scroll area would have handled natively, successfully diverting them to the custom wheelEvent to maintain zoom behavior regardless of image size.

The moment the app boots, it pauses for 0 milliseconds—allowing the CPU to finish evaluating layout margins—and then fires updateImage() so the "Fit" scale maps perfectly to the physical window dimensions. main.cpp was updated to detect images with an alpha channel and automatically composite them onto a solid white background.

The buttons or scroll area were "stealing" the focus. This was fixed by setting setFocusPolicy(Qt::NoFocus) on all navigation buttons, zoom buttons, and the QScrollArea. This ensured the main ImageViewer window always retained keyboard focus, allowing keyPressEvent to trigger correctly even after user interaction with the UI.

The application was updated to support passing a directory as a single argument. In main, if the single argument pointed to a directory, we automatically search for files with common image extensions (.jpg, .png, etc.). All discovered images are then loaded into the viewer.

The taskbar layout was restructured and the information display was added. The 50px bottom bar was divided into three zones: metadata on the left , centered navigation/zoom buttons in the middle and status messages on the right.

Client: Centre the metadata text inside the left zone (i.e. the centre of the metadata text should be halfway between the left edge of the window and the left edge of the leftmost button)

To address this, the alignment of the left zone was changed to Qt::AlignCenter and its stretch factor was set to 1. Since the center button container used a stretch factor of 0, the left and right zones shared the remaining space equally, centering the metadata halfway between the window edge and the buttons.

Success/error status messages appear in `infoLabelRight` added with `Qt::AlignCenter` so its text is centered inside the right zone only and persist until overwritten. Ensured the right-info label is centered strictly within its own zone.

Client: It is absolutely not negotiable that the buttons need to be centered — I implemented the Symmetric Spacer Strategy in main.cpp:

- `infoLabelLeft` and `infoLabelRight` now use `QSizePolicy::Ignored` horizontally and `setMinimumWidth(0)`, so their text won't bias initial layout sizing.
- `buttonContainer` is set to `QSizePolicy::Minimum` x `QSizePolicy::Fixed`, and is added with stretch 0; left/right labels are added with stretch 1.
- `infoLabelLeft` alignment is `Qt::AlignLeft | Qt::AlignVCenter`; `infoLabelRight` is `Qt::AlignRight | Qt::AlignVCenter`.

Later, we also implemented the following changes: when we encounter a file that falls into some unsupported extensions, we use magick.exe to convert it to png and save it to %temp%<hash of the file>.png, before checking if the converted version already exists. Once we find the converted png, we load that and pretend as if nothing happened.

Client: Implement a 1. drag-to-scroll feature, and a 2. Zoom to Cursor, which treats the pointer's coordinates as the fixed anchor point or Zoom Pivot.

Added the QPoint lastMousePos and bool isPanning private variables to the ImageViewer class in the private section to track panning state. Updated your eventFilter to include MouseButtonPress, MouseMove, and MouseButtonRelease. I added panning state, implemented drag-to-scroll in the viewport event filter, and implemented zoom-to-mouse, updating wheelEvent and zoomOffset to use the viewport mouse position.
