#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label("Hello World");
    label.setWindowTitle("Waffle");
    label.resize(320, 200);
    label.setAlignment(Qt::AlignCenter);
    label.show();

    return app.exec();
}
