This is the story of how the imager.py application was ported to C++, to address the limitations of Python in terms of performance and UI responsiveness when handling large images.

Client: Make an Image Viewer app that loads image(s), opens and displays it, supports window resizing, always keeping the image as big as possible whithin bounds and zooming in and out as needed to keep it that way.

And thus, it begins. main.cpp was created to implement a custom QLabel subclass called ImageViewer which manages maintaining the original image in memory using a QPixmap. When an image is provided, the viewer uses loadImage() to read the file into memory and initially display it scaled to the size of the viewer. By overriding the resizeEvent() method, the application is notified whenever the window changes size. When this happens, the viewer re-calculates the scaled size based on the new dimensions, and the constant aspect ratio (Qt::KeepAspectRatio), smoothly re-rendering the output (Qt::SmoothTransformation). The window title updates to the filename, and the background gracefully fills with #eef3fa to clearly denote the image edges.

Client: Make it so that it exists in three different states for zoom behaviour: fit, one and custom. In the fit state, it acts like it does now. In the one state, it is pixel-perfect with the screen i.e. 100% zoom. Scrolling up or down moves the app from the current state to the custom state. 0.1 is added or subtracted from the zoom level on scrolling up and down. The app starts in fit mode. Add a taskbar on the bottom 50 pixels in height stretching the width. In there add two buttons, for switching back to Fit or One mode.

And so, ImageViewer was refactored from QLabel to QWidget. This allowed for a vertical layout (QVBoxLayout) to hold the image at the top and the taskbar at the bottom. An enum was created, ZoomState { Fit, One, Custom }, and logic was implemented so the app started in Fit mode by default, maintaining aspect ratio. One mode switched the image to its original unscaled dimensions, while any mouse scroll moved it directly into Custom mode. The wheelEvent was overridden to add or subtract exactly 0.1 to the zoomLevel when pushed up or down, triggering updateImage to apply the scale. Finally, a 50px tall horizontal layout taskbar was docked at the bottom with "Fit" and "One" QPushButtons hooked to refresh the view.

At this point, a bug was discovered: when zooming in, the window itself was resizing to fit the new image dimensions instead of keeping the window size fixed and allowing the image to be zoomed beyond it. This was an unintended consequence of how the QLabel handled large pixmaps within the layout. I fixed it by wrapping the image label in a QScrollArea, which allows the image to be zoomed beyond the window size without resizing the window itself. Now when you zoom in, you can scroll to see the rest of the image, and the window stays fixed at its current size.

Client: Ok nice but make the scrollbars invisible. Also when the image goes out of bounds, the scrolling keypresses go towards scrolling up and down instead of our zoom functionality.

The updates were made to hide the scrollbars and ensure scrolling always triggered zooming. The horizontal and vertical scrollbars were set to be invisible using Qt::ScrollBarAlwaysOff. Additionally, a global scroll hook was established by setting an eventFilter on the QScrollArea's viewport. This intercepted any wheel events the scroll area would have handled natively, successfully diverting them to the custom wheelEvent to maintain zoom behavior regardless of image size.

Client: The app seems to open in some in between zoom level and when I press any button or resize it, it's like it suddenly remembers, and snaps. Figure out why.

The root cause was identified: when loadImage() was called, the widget had not yet been drawn, meaning the UI layout wasn't fully calculated and the viewport used a default size. This was fixed by adding a QTimer and capturing the initial showEvent. Now, the moment the app boots, it pauses for 0 milliseconds—allowing the CPU to finish evaluating layout margins—and then fires updateImage() so the "Fit" scale maps perfectly to the physical window dimensions.

Client: Great, add in a special case where if it detects an RGBA image and it adds a white background.

main.cpp was updated to detect images with an alpha channel and automatically composite them onto a solid white background. #include <QPainter> was added to allow for image compositing, and updateImage() was modified to check scaledPixmap.hasAlpha(). If detected, a new QPixmap was created and filled with Qt::white, with the original image painted over it to ensure transparent PNGs displayed with a clean background.

Client: Add in support for multiple arguments. E.g. image1.png image2.jpeg, etc. When it opens up, it should be the first image, but pressing left or right arrow to move between the list. Also add in "Next" and "Previous" buttons in the toolbar for that. When you move to an image, it should reset zoom mode to Fit.

Support for multiple image arguments and navigation was added. The app began collecting all command-line arguments into a list, displaying the first image upon opening. "Previous" and "Next" buttons were added to the toolbar, and a keyboard shortcut was implemented to use the Left and Right arrow keys to move between images. Furthermore, the zoom mode was set to automatically reset to Fit every time a new image was loaded.

Client: Nice but the keyboard shortcut isn't working

The reason the shortcuts weren't working was that the buttons or scroll area were "stealing" the focus. This was fixed by setting setFocusPolicy(Qt::NoFocus) on all navigation buttons, zoom buttons, and the QScrollArea. This ensured the main ImageViewer window always retained keyboard focus, allowing keyPressEvent to trigger correctly even after user interaction with the UI.

The application was updated to support passing a directory as a single argument. In main, if the single argument pointed to a directory, we automatically search for files with common image extensions (.jpg, .png, etc.). All discovered images are then loaded into the viewer, allowing for seamless navigation while maintaining support for individual file paths.

Client: Ok in the taskbar, centre the buttons. Separate the taskbar into three zones. The button zone, exactly in the middle, and two info zones on the left. In the left info zone, display text of the like "1/17\t0.317x\t3000 x 3000". 1/17 is the current image index/number of images to cycle through, 0.317x is the current zoom level, and then is the current image resolution.

The taskbar layout was restructured and the information display was added. The 50px bottom bar was divided into three zones: metadata on the left, centered navigation/zoom buttons in the middle. The metadata text was made to update instantly upon scrolling, resizing, or switching images, and a monospace font was utilized to keep the alignment stable. 

Client: Centre the metadata text inside the left zone (i.e. the centre of the metadata text should be halfway between the left edge of the window and the left edge of the leftmost button)

To address this, the alignment of the left zone was changed to Qt::AlignCenter and its stretch factor was set to 1. Since the center button container used a stretch factor of 0, the left and right zones shared the remaining space equally, centering the metadata halfway between the window edge and the buttons.

Client: Add the following buttons to the toolbar: A "copy", minus and plus button to zoom in and out, a "delete" button to run C:\tools\delet.exe (my custom delete utility) on the file, "reload" and an "Edit" button to launch mspaint.exe with the current image file name as the first argument.

And so they were added. The copy logic was moved to an external file copyimage.cpp. Added copyimage.cpp as a separate executable in CMakeLists.txt and linked the necessary Windows libraries (gdiplus, ole32).

Success/error status messages now appear in `infoLabelRight` added with `Qt::AlignCenter` so its text is centered inside the right zone only and persist until overwritten. Ensured the right-info label is centered strictly within its own zone.

Client: It is absolutely not negotiable that the buttons need to be centered, yet when the window is smaller than 1043 pixels, the buttons are allowed to slide to the left. Instead, the left info area should shrink.

Done — I implemented the Symmetric Spacer Strategy in main.cpp:

- `infoLabelLeft` and `infoLabelRight` now use `QSizePolicy::Ignored` horizontally and `setMinimumWidth(0)`, so their text won't bias initial layout sizing.
- `buttonContainer` is set to `QSizePolicy::Minimum` x `QSizePolicy::Fixed`, and is added with stretch 0; left/right labels are added with stretch 1.
- `infoLabelLeft` alignment is `Qt::AlignLeft | Qt::AlignVCenter`; `infoLabelRight` is `Qt::AlignRight | Qt::AlignVCenter`.

Later, we also implemented the following changes: when we encounter a file that falls into those extensions, we use magick.exe to convert it to png and save it to %temp%<hash of the file>.png, oh and before that, we check if the converted version already exists. Once we find the png converted version, we load that and pretend as if nothing happened.

Client: Implement a 1. drag-to-scroll feature, and a 2. Zoom to Cursor, which treats the pointer's coordinates as the fixed anchor point or Zoom Pivot.

Added the QPoint lastMousePos and bool isPanning private variables to the ImageViewer class in the private section to track panning state. Updated your eventFilter to include MouseButtonPress, MouseMove, and MouseButtonRelease. I added panning state, implemented drag-to-scroll in the viewport event filter, and implemented zoom-to-mouse, updating wheelEvent and zoomOffset to use the viewport mouse position.
