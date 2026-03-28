#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include <QPainter>
#include <QStringList>
#include <QDir>

class ImageViewer : public QWidget {
public:
    enum class ZoomState { Fit, One, Custom };

    ImageViewer() {
        setWindowTitle("Waffle");
        resize(800, 600);
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
        
        auto *btnPrev = new QPushButton("Previous");
        auto *btnNext = new QPushButton("Next");
        auto *btnFit = new QPushButton("Fit");
        auto *btnOne = new QPushButton("One");
        
        btnPrev->setFocusPolicy(Qt::NoFocus);
        btnNext->setFocusPolicy(Qt::NoFocus);
        btnFit->setFocusPolicy(Qt::NoFocus);
        btnOne->setFocusPolicy(Qt::NoFocus);
        scrollArea->setFocusPolicy(Qt::NoFocus);
        
        QString btnStyle = "background-color: #444; border: 1px solid #555; padding: 5px 15px; border-radius: 3px;";
        btnPrev->setStyleSheet(btnStyle);
        btnNext->setStyleSheet(btnStyle);
        btnFit->setStyleSheet(btnStyle);
        btnOne->setStyleSheet(btnStyle);

        bottomLayout->addWidget(btnPrev);
        bottomLayout->addWidget(btnNext);
        bottomLayout->addSpacing(20);
        bottomLayout->addWidget(btnFit);
        bottomLayout->addWidget(btnOne);
        bottomLayout->addStretch();
        
        mainLayout->addWidget(bottomBar);

        connect(btnPrev, &QPushButton::clicked, this, &ImageViewer::previousImage);
        connect(btnNext, &QPushButton::clicked, this, &ImageViewer::nextImage);
        connect(btnFit, &QPushButton::clicked, this, [this]() {
            setZoomState(ZoomState::Fit);
        });
        connect(btnOne, &QPushButton::clicked, this, [this]() {
            setZoomState(ZoomState::One);
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
        if (originalPixmap.load(fileName)) {
            setWindowTitle(QFileInfo(fileName).fileName() + " - Waffle");
            setZoomState(ZoomState::Fit);
        } else {
            imageLabel->setText("Failed to load image: " + QFileInfo(fileName).fileName());
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
        if (event->type() == QEvent::Wheel) {
            auto *wheelEv = static_cast<QWheelEvent*>(event);
            this->wheelEvent(wheelEv);
            return true;
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
        if (originalPixmap.isNull()) return;

        int angle = event->angleDelta().y();
        if (angle > 0) {
            zoomLevel += 0.2;
        } else if (angle < 0) {
            zoomLevel -= 0.2;
            if (zoomLevel < 0.1) zoomLevel = 0.1;
        }
        zoomState = ZoomState::Custom;
        updateImage();
    }

private:
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
    }

    QLabel *imageLabel;
    QScrollArea *scrollArea;
    QPixmap originalPixmap;
    ZoomState zoomState;
    double zoomLevel;
    QStringList images;
    int currentImageIndex;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ImageViewer viewer;

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
