#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QLabel label("YT Kids — coming soon");
    label.show();
    return app.exec();
}
