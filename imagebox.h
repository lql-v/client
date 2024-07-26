#ifndef IMAGEBOX_H
#define IMAGEBOX_H

#include <QWidget>
#include <QScreen>
namespace Ui {
class ImageBox;
}

class ImageBox : public QWidget
{
    Q_OBJECT

public:
    explicit ImageBox(QWidget *parent = nullptr);
    void setLabelImg(QByteArray imgdata);
    ~ImageBox();

private:
    Ui::ImageBox *ui;
};

#endif // IMAGEBOX_H
