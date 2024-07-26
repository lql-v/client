#include "imagebox.h"
#include "ui_imagebox.h"

ImageBox::ImageBox(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageBox)
{
    ui->setupUi(this);
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    move(screenGeometry.center() - rect().center());
}

// 展示图片
void ImageBox::setLabelImg(QByteArray imgdata)
{
    QPixmap pixmap;
    pixmap.loadFromData(imgdata);
    ui->labelImage->setFixedSize(pixmap.size());
    ui->labelImage->setAlignment(Qt::AlignCenter);
    ui->labelImage->setPixmap(pixmap);
}

ImageBox::~ImageBox()
{
    delete ui;
}
