#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>  // You will also need this for QMouseEvent in the event filter
#include <QPainter>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>
#include <QSettings>

class ImageViewer : public QWidget {
public:
    void setConfigPath(const QString& path) {
        configPath = path;
    }

    enum class ZoomState { Fit, One, Custom };

    ImageViewer() {
        setWindowTitle("Waffle");
        resize(1043, 600);
        setStyleSheet("background-color: #eef3fa; color: black;");

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        imageLabel = new QLabel;
        imageLabel->setAlignment(Qt::AlignCenter);

        scrollArea = new QScrollArea;
        scrollArea->setWidget(imageLabel);
        scrollArea->setAlignment(Qt::AlignCenter);
        scrollArea->setWidgetResizable(false);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->viewport()->installEventFilter(this);
        mainLayout->addWidget(scrollArea);

        auto *bottomBar = new QWidget;
        bottomBar->setFixedHeight(50);
        bottomBar->setStyleSheet("background-color: #2b2b2b; color: white;");
        auto *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(10, 0, 10, 0);

        // Left info zone (between left edge and buttons)
        infoLabelLeft = new QLabel;
        infoLabelLeft->setStyleSheet("font-family: 'Courier New', monospace; color: white; font-size: 18px;");
        infoLabelLeft->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        infoLabelLeft->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        infoLabelLeft->setMinimumWidth(0);
        bottomLayout->addWidget(infoLabelLeft, 1);

        // Center button zone
        auto *buttonContainer = new QWidget;
        auto *buttonLayout = new QHBoxLayout(buttonContainer);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->setSpacing(10);

        auto *btnPrev = new QPushButton("\u2190"); // left arrow
        auto *btnNext = new QPushButton("\u2192"); // right arrow
        auto *btnFit = new QPushButton("\u26f6");
        auto *btnOne = new QPushButton("1:1");
        // auto *btnMinus = new QPushButton("-");
        // auto *btnPlus = new QPushButton("+");
        auto *btnCopy = new QPushButton("\u2398");
        auto *btnReload = new QPushButton("\u27f3");
        auto *btnEdit = new QPushButton("\u270e"); // pencil icon
        auto *btnDelete = new QPushButton("X");
        
        btnPrev->setFocusPolicy(Qt::NoFocus);
        btnNext->setFocusPolicy(Qt::NoFocus);
        btnFit->setFocusPolicy(Qt::NoFocus);
        btnOne->setFocusPolicy(Qt::NoFocus);
        // btnMinus->setFocusPolicy(Qt::NoFocus);
        // btnPlus->setFocusPolicy(Qt::NoFocus);
        btnCopy->setFocusPolicy(Qt::NoFocus);
        btnReload->setFocusPolicy(Qt::NoFocus);
        btnEdit->setFocusPolicy(Qt::NoFocus);
        btnDelete->setFocusPolicy(Qt::NoFocus);
        scrollArea->setFocusPolicy(Qt::NoFocus);
        
        QString btnStyle = "background-color: #444; border: 1px solid #555; padding: 5px 15px; border-radius: 3px;";
        btnPrev->setStyleSheet(btnStyle);
        btnNext->setStyleSheet(btnStyle);
        btnFit->setStyleSheet(btnStyle);
        btnOne->setStyleSheet(btnStyle);
        // btnMinus->setStyleSheet(btnStyle);
        // btnPlus->setStyleSheet(btnStyle);
        btnCopy->setStyleSheet(btnStyle);
        btnReload->setStyleSheet(btnStyle);
        btnEdit->setStyleSheet(btnStyle);
        btnDelete->setStyleSheet(btnStyle);

        buttonLayout->addWidget(btnPrev);
        buttonLayout->addWidget(btnNext);
        buttonLayout->addWidget(btnFit);
        buttonLayout->addWidget(btnOne);
        // buttonLayout->addWidget(btnPlus);
        // buttonLayout->addWidget(btnMinus);
        buttonLayout->addWidget(btnCopy);
        buttonLayout->addWidget(btnReload);
        buttonLayout->addWidget(btnEdit);
        buttonLayout->addWidget(btnDelete);

        buttonContainer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        bottomLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);

        // Right info zone (between buttons and right edge)
        infoLabelRight = new QLabel;
        infoLabelRight->setStyleSheet("color: white; font-size: 18px;");
        infoLabelRight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        infoLabelRight->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        infoLabelRight->setMinimumWidth(0);
        bottomLayout->addWidget(infoLabelRight, 1, Qt::AlignCenter);
        
        mainLayout->addWidget(bottomBar);

        connect(btnPrev, &QPushButton::clicked, this, &ImageViewer::previousImage);
        connect(btnNext, &QPushButton::clicked, this, &ImageViewer::nextImage);
        connect(btnFit, &QPushButton::clicked, this, [this]() {
            setZoomState(ZoomState::Fit);
        });
        connect(btnOne, &QPushButton::clicked, this, [this]() {
            setZoomState(ZoomState::One);
        });
        // connect(btnMinus, &QPushButton::clicked, this, [this]() { zoomOffset(-1); });
        // connect(btnPlus, &QPushButton::clicked, this, [this]() { zoomOffset(1); })
        connect(btnCopy, &QPushButton::clicked, this, [this]() {
            if (!images.isEmpty()) {
                QString exePath = QCoreApplication::applicationDirPath() + "/copyimage.exe";
                QProcess::startDetached(exePath, QStringList() << images[currentImageIndex]);
                infoLabelRight->setText("Copied to clipboard!");
            }
        });
        connect(btnReload, &QPushButton::clicked, this, [this]() {
            if (!images.isEmpty()) {
                loadImage(images[currentImageIndex]);
                infoLabelRight->setText("Reloaded!");
            }
        });
        connect(btnEdit, &QPushButton::clicked, this, [this]() {
            if (!images.isEmpty()) {
                QProcess::startDetached("mspaint.exe", QStringList() << images[currentImageIndex]);
                infoLabelRight->setText("Opened in editor");
            }
        });
        connect(btnDelete, &QPushButton::clicked, this, [this]() {
            if (!images.isEmpty()) {
                QSettings configFile(configPath, QSettings::IniFormat);
                QString deleteExe = configFile.value("deleteCommand", "cmd /c del").toString();
                // We use QProcess::execute (blocking) or check startDetached return value, 
                // but since delete might be quick, let's use a non-detached process to check success.
                QProcess proc;
                proc.start(deleteExe, QStringList() << images[currentImageIndex]);
                if (proc.waitForFinished() && proc.exitCode() == 0) {
                    infoLabelRight->setText("Deleted!");
                } else {
                    QString errorMsg;
                    if (proc.error() == QProcess::FailedToStart) {
                        errorMsg = "Exe not found!";
                    } else {
                        errorMsg = "Exit code " + QString::number(proc.exitCode());
                    }
                    infoLabelRight->setText("Fail: " + errorMsg);
                }
            }
        });

        zoomState = ZoomState::Fit;
        zoomLevel = 1.0;
        currentImageIndex = 0;
    }

    void loadImages(const QStringList &fileNames) {
        images = fileNames;
        currentImageIndex = 0;
        if (!images.isEmpty()) {
            loadImage(images[currentImageIndex]);
        } else {
            imageLabel->setText("No images provided.");
        }
    }

    void loadImage(const QString &fileName) {
        QFileInfo fi(fileName);

        // Determine whether the file needs proxy conversion via ImageMagick
        QString ext = fi.suffix().toLower();
        static const QSet<QString> proxyExts = { "heic", "heif", "avif", "jp2", "j2k", "jpx" };

        QString targetLoadPath = fileName;
        if (proxyExts.contains(ext)) {
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly)) {
                imageLabel->setText("Failed to open file: " + fi.fileName());
                return;
            }

            QCryptographicHash hash(QCryptographicHash::Md5);
            while (!file.atEnd()) {
                QByteArray chunk = file.read(8192);
                hash.addData(chunk);
            }
            QString hex = hash.result().toHex();

            QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            QDir().mkpath(tempDir);
            QString tempPath = QDir(tempDir).filePath(hex + ".png");

            if (!QFileInfo::exists(tempPath)) {
                QSettings configFile(configPath, QSettings::IniFormat);
                QString magickPath = configFile.value("magickPath", "C:\\Program Files\\ImageMagick-7.1.1-Q16-HDRI\\magick.exe").toString();
                int rc = QProcess::execute(magickPath, QStringList() << fileName << tempPath);
                if (rc != 0) {
                    imageLabel->setText("Failed to convert: " + fi.fileName());
                    return;
                }
            }

            targetLoadPath = tempPath;
        }

        if (originalPixmap.load(targetLoadPath)) {
            // Use the original filename for UI metadata, but pixels come from the converted PNG when necessary
            setWindowTitle(fi.fileName() + " - Waffle");
            setZoomState(ZoomState::Fit);
        } else {
            imageLabel->setText("Failed to load image: " + fi.fileName());
        }
    }

    void nextImage() {
        if (images.size() <= 1) return;
        currentImageIndex = (currentImageIndex + 1) % images.size();
        loadImage(images[currentImageIndex]);
    }

    void previousImage() {
        if (images.size() <= 1) return;
        currentImageIndex = (currentImageIndex - 1 + images.size()) % images.size();
        loadImage(images[currentImageIndex]);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Left) {
            previousImage();
        } else if (event->key() == Qt::Key_Right) {
            nextImage();
        } else {
            QWidget::keyPressEvent(event);
        }
    }

    void showEvent(QShowEvent *event) override {
        QWidget::showEvent(event);
        // Force an update once the layout and viewport have their final initial sizes calculated
        QTimer::singleShot(0, this, [this]() {
            if (zoomState == ZoomState::Fit) {
                updateImage();
            }
        });
    }

    bool eventFilter(QObject *obj, QEvent *event) override {
        if (obj == scrollArea->viewport()) {
            if (event->type() == QEvent::MouseButtonPress) {
                auto *mouseEv = static_cast<QMouseEvent*>(event);
                if (mouseEv->button() == Qt::LeftButton) {
                    isPanning = true;
                    lastMousePos = mouseEv->pos();
                    scrollArea->setCursor(Qt::ClosedHandCursor);
                    return true;
                }
            } else if (event->type() == QEvent::MouseMove) {
                auto *mouseEv = static_cast<QMouseEvent*>(event);
                if (isPanning) {
                    QPoint delta = mouseEv->pos() - lastMousePos;
                    lastMousePos = mouseEv->pos();
                    scrollArea->horizontalScrollBar()->setValue(scrollArea->horizontalScrollBar()->value() - delta.x());
                    scrollArea->verticalScrollBar()->setValue(scrollArea->verticalScrollBar()->value() - delta.y());
                    return true;
                }
            } else if (event->type() == QEvent::MouseButtonRelease) {
                auto *mouseEv = static_cast<QMouseEvent*>(event);
                if (mouseEv->button() == Qt::LeftButton) {
                    isPanning = false;
                    scrollArea->setCursor(Qt::ArrowCursor);
                    return true;
                }
            } else if (event->type() == QEvent::Wheel) {
                auto *wheelEv = static_cast<QWheelEvent*>(event);
                // forward to our wheelEvent so we can use viewport-relative coords
                this->wheelEvent(wheelEv);
                return true;
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        if (zoomState == ZoomState::Fit) {
            updateImage();
        }
    }

    void wheelEvent(QWheelEvent *event) override {
        // Mouse position relative to the scrollArea viewport
        QPoint mousePos = event->position().toPoint();
        zoomOffset(event->angleDelta().y(), mousePos);
    }

private:
    void zoomOffset(int direction, QPoint mousePos) {
        if (originalPixmap.isNull()) return;
        // 1. Determine old zoom
        double oldZoom;
        if (zoomState == ZoomState::Fit) {
            oldZoom = (double)imageLabel->pixmap().width() / originalPixmap.width();
        } else if (zoomState == ZoomState::One) {
            oldZoom = 1.0;
        } else {
            oldZoom = zoomLevel;
        }

        // 2. Capture current scrollbar positions
        QPoint oldScroll(scrollArea->horizontalScrollBar()->value(), scrollArea->verticalScrollBar()->value());

        // 3. Compute image coordinate under the mouse (in original pixmap space)
        QPointF imagePos = QPointF(mousePos + oldScroll) / oldZoom;

        // 4. Compute new zoom level
        if (direction > 0) {
            if (oldZoom < 0.1) zoomLevel = oldZoom / 0.8;
            else zoomLevel = oldZoom + 0.1;
        } else if (direction < 0) {
            if (oldZoom < 0.2) zoomLevel = oldZoom * 0.8;
            else zoomLevel = oldZoom - 0.1;
            double minZoom = 1.0 / std::min(originalPixmap.width(), originalPixmap.height());
            if (zoomLevel < minZoom) zoomLevel = minZoom;
        } else {
            return;
        }

        zoomState = ZoomState::Custom;

        // 5. Apply the new scale first so scroll ranges update
        updateImage();

        // 6. Compute new content position for the same image point and set scrollbars
        QPointF newContentPos = imagePos * zoomLevel;
        scrollArea->horizontalScrollBar()->setValue(static_cast<int>(newContentPos.x() - mousePos.x()));
        scrollArea->verticalScrollBar()->setValue(static_cast<int>(newContentPos.y() - mousePos.y()));
    }

    void setZoomState(ZoomState state) {
        zoomState = state;
        if (state == ZoomState::One) {
            zoomLevel = 1.0;
        }
        updateImage();
    }

    void updateImage() {
        if (originalPixmap.isNull()) return;

        QPixmap scaledPixmap;
        if (zoomState == ZoomState::Fit) {
            QSize viewportSize = scrollArea->viewport()->size();
            scaledPixmap = originalPixmap.scaled(viewportSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        } else if (zoomState == ZoomState::One) {
            scaledPixmap = originalPixmap;
        } else {
            QSize newSize = originalPixmap.size() * zoomLevel;
            if (newSize.width() < 1) newSize.setWidth(1);
            if (newSize.height() < 1) newSize.setHeight(1);
            scaledPixmap = originalPixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // Handle RGBA images by adding a white background
        if (scaledPixmap.hasAlpha()) {
            QPixmap background(scaledPixmap.size());
            background.fill(Qt::white);
            QPainter painter(&background);
            painter.drawPixmap(0, 0, scaledPixmap);
            painter.end();
            scaledPixmap = background;
        }

        imageLabel->setPixmap(scaledPixmap);
        imageLabel->resize(scaledPixmap.size());

        updateInfoDisplay();
    }

    void updateInfoDisplay() {
        if (originalPixmap.isNull()) {
            infoLabelLeft->clear();
            infoLabelRight->clear();
            return;
        }

        // Current image index / total images
        QString indexStr = QString("%1/%2").arg(currentImageIndex + 1).arg(images.size());

        // Zoom level calculation
        double actualZoom;
        if (zoomState == ZoomState::Fit) {
            actualZoom = (double)imageLabel->pixmap().width() / originalPixmap.width();
        } else if (zoomState == ZoomState::One) {
            actualZoom = 1.0;
        } else {
            actualZoom = zoomLevel;
        }

        QString zoomStr = QString("%1x").arg(actualZoom, 0, 'f', 3);
        QString resolutionStr = QString("%1 x %2").arg(originalPixmap.width()).arg(originalPixmap.height());

        infoLabelLeft->setText(QString("%1   %2   %3").arg(indexStr, zoomStr, resolutionStr));
    }

    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QLabel *infoLabelLeft;
    QLabel *infoLabelRight;
    QPixmap originalPixmap;
    ZoomState zoomState;
    double zoomLevel;
    QStringList images;
    int currentImageIndex;
    QPoint lastMousePos;
    bool isPanning = false;
    QString configPath;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ImageViewer viewer;

    // Determine config file path
    QString configPath;
    const char* envConfig = getenv("WaffleImager_config");
    if (envConfig) {
        configPath = QString::fromLocal8Bit(envConfig);
    } else {
        configPath = QCoreApplication::applicationDirPath() + "/config.ini";
    }

    viewer.setConfigPath(configPath);

    if (argc > 1) {
        QStringList args;
        if (argc == 2) {
            QString path = QString::fromLocal8Bit(argv[1]);
            QFileInfo checkFile(path);
            if (checkFile.isDir()) {
                QDir dir(path);
                QStringList filters;
                filters << "*.jpg" << "*.jpeg" << "*.jpe" << "*.jfif" << "*.png" << "*.gif" << "*.webp" 
        << "*.bmp" << "*.dib" << "*.ico" << "*.tiff" << "*.tif" << "*.svg" << "*.svgz" 
        << "*.heic" << "*.heif" << "*.avif" << "*.tga" << "*.psd" << "*.dng" << "*.hdr" 
        << "*.exr" << "*.jp2" << "*.j2k" << "*.jpx" << "*.jpm" << "*.mj2" << "*.pnm" 
        << "*.pbm" << "*.pgm" << "*.ppm" << "*.xbm" << "*.xpm" << "*.dds";
                QFileInfoList list = dir.entryInfoList(filters, QDir::Files);
                for (const QFileInfo &fileInfo : list) {
                    args << fileInfo.absoluteFilePath();
                }
            } else {
                args << path;
            }
        } else {
            for (int i = 1; i < argc; ++i) {
                args << QString::fromLocal8Bit(argv[i]);
            }
        }
        viewer.loadImages(args);
    } else {
        viewer.loadImages(QStringList());
    }

    viewer.show();

    return app.exec();
}
