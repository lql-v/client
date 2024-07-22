#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    //pswLineEdit为密码输入框的name
    ui->setupUi(this);
    ui->lineEditPassword->setEchoMode(QLineEdit::Password);

    // 固定大小
    setFixedSize(this->width(), this->height());

    // 创建与服务端连接
    m_websock = new ClientWebSocket(this);
    m_uploadModel = new QStringListModel(this);
    m_cloudModel = new QStringListModel(this);

    connect(ui->listViewSelectLists, &QListView::doubleClicked,
            this, &MainWindow::doubleClickOnSelectedListView);

    connect(m_websock, &ClientWebSocket::imgListReceived, this, &MainWindow::updateCloudList);
    connect(ui->listViewCloudLists, &QListView::doubleClicked, this, &MainWindow::doubleClickOnCloudListView);
    connect(m_websock, &ClientWebSocket::imgDataReceived, this, &MainWindow::showImg);
}

MainWindow::~MainWindow()
{
    if(m_websock != nullptr)
    {
        delete m_websock;
        m_websock = nullptr;
    }
    if(m_uploadModel != nullptr)
    {
        delete m_uploadModel;
        m_uploadModel = nullptr;
    }
    if(m_cloudModel != nullptr)
    {
        delete m_cloudModel;
        m_cloudModel = nullptr;
    }
    delete ui;
}

QJsonObject MainWindow::getInput()
{
    // 获取用户输入数据
    QString name = ui->lineEditUserName->text();
    QString pwd = ui->lineEditPassword->text();

    QJsonObject userdata;

    // 创建Json数据
    userdata.insert("password", pwd);
    userdata.insert("username", name);
    return userdata;
}

void MainWindow::on_pushButtonLogin_clicked()
{
    // 已经登陆直接返回
    if(m_websock->isLogined())
    {
        return;
    }

    // 获取输入内容
    QJsonObject userdata = getInput();

    if(userdata.isEmpty())
    {
        MsgBox *box = new MsgBox(this, "请正确填写账号密码", "错误");
        box->exec();
        return;
    }

    // 创建Json
    QJsonObject msg;
    msg.insert("userdata", userdata);
    msg.insert("request", "login");

    // 记录姓名，后续会用
    m_username = ui->lineEditUserName->text();

    // 通过socket写出msg
    m_websock->sendLoginSignupMsg(msg);

    return;
}

void MainWindow::on_pushButtonSignUp_clicked()
{
    // 获取输入内容
    QJsonObject userdata = getInput();

    if(userdata.isEmpty())
    {
        MsgBox *box = new MsgBox(this, "请正确填写账号密码", "错误");
        box->exec();
        return;
    }

    // 创建Json
    QJsonObject msg;
    msg.insert("userdata", userdata);
    msg.insert("request", "login");

    // 通过socket写出msg
    m_websock->sendLoginSignupMsg(msg);
    return;
}

void MainWindow::on_pushButtonSelectFile_clicked()
{
    // 检查登陆状态
    if(!m_websock->isLogined())
    {
        MsgBox *box = new MsgBox(this, "未登陆，请先登陆", "警告");
        box->exec();
        return;
    }

    // 获取当前应用程序路径
    QString curPath = QCoreApplication::applicationDirPath();

    // 获取已选择的路径和原路径拼接
    QStringList strList = m_uploadModel->stringList();
    QStringList filenames = QFileDialog::getOpenFileNames(this, "选择待上传的图片文件", curPath, "图片文件(*.png)");
    // 过滤相同的
    for (const QString &item : filenames) {
        if (!strList.contains(item)) {
            strList.append(item);
        }
    }
    m_uploadModel->setStringList(strList);

    // 展示到ui
    ui->listViewSelectLists->setModel(m_uploadModel);

    // 选中最后一行
    QModelIndex index = m_uploadModel->index(m_uploadModel->rowCount(),0);
    ui->listViewSelectLists->setCurrentIndex(index);
}

void MainWindow::on_pushButtonUpload_clicked()
{
    // 检查登陆状态
    if(!m_websock->isLogined())
    {
        MsgBox *box = new MsgBox(this, "未登陆，请先登陆", "警告");
        box->exec();
        return;
    }

    // 获取字符串列表
    QStringList strList = m_uploadModel->stringList();

    // 对每个文件操作
    for (int i = 0; i < strList.size(); ++i)
    {
        // 读取图片二进制数据
        QFile file(strList[i]);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "打开文件失败：" << file.errorString();
            return;
        }
        // 读取文件内容
        QByteArray imageData = file.readAll();
        file.close();

        // 用aes128加密
        QByteArray key ("FileStoreService");
        QAESEncryption aesEncryption(QAESEncryption::AES_128, QAESEncryption::CBC);
        QByteArray encryptedData = aesEncryption.encode(imageData, key, key);

        // 准备Json
        QFileInfo fileInfo(strList[i]);
        QJsonObject userdata;
        userdata.insert("username", m_username);
        userdata.insert("imgname", fileInfo.fileName());
        userdata.insert("imgdata", QString(encryptedData));
        QJsonObject msg;
        msg.insert("userdata", userdata);

        // 发送数据至服务端
        m_websock->uploadImg(msg);
    }
    // 清空list和model
    m_uploadModel->setStringList(QStringList());
    ui->listViewSelectLists->update();
}

void MainWindow::updateCloudList(QStringList imgs)
{
    m_cloudModel->setStringList(imgs);
    // 展示到ui
    ui->listViewSelectLists->setModel(m_uploadModel);
    // 选中最后一行
    QModelIndex index = m_uploadModel->index(m_uploadModel->rowCount(),0);
    ui->listViewSelectLists->setCurrentIndex(index);
}

void MainWindow::on_pushButtonFlush_clicked()
{
    if(!m_websock->isLogined())
    {
        MsgBox *box = new MsgBox(this, "未登陆，请先登陆", "警告");
        box->exec();
        return;
    }

    m_websock->getCloudList(m_username);
}

void MainWindow::doubleClickOnSelectedListView(const QModelIndex &index)
{
    m_uploadModel->removeRow(index.row());
}

void MainWindow::doubleClickOnCloudListView(const QModelIndex &index)
{
    // 获取选中行名字
    QString imgName = m_cloudModel->data(index, Qt::DisplayRole).toString();

    // 请求图片数据
    m_websock->getImg(m_username, imgName);
}

void MainWindow::showImg(QByteArray imgdata)
{
    QPixmap pixmap;
    pixmap.loadFromData(imgdata);
    QLabel *label = new QLabel(this);
    label->setPixmap(pixmap);
    label->resize(pixmap.height(),pixmap.width());
    label->show();
}
