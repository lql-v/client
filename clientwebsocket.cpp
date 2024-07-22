#include "clientwebsocket.h"

ClientWebSocket::ClientWebSocket(QObject *parent)
    : QObject{parent}
{
    // 创建tcp套接字
    m_tcpsock = new QTcpSocket(this);

    // 断开连接事件
    QObject::connect(m_tcpsock, &QTcpSocket::disconnected, parent, [parent](){
        // 在槽函数中添加条件判断，比如窗口是否正在关闭
        if (!static_cast<QWidget*>(parent)->isVisible()) {
            return;
        }
        MsgBox *box = new MsgBox(static_cast<QWidget*>(parent), "服务器已离线，请等待服务器开启！");
        box->exec();
        exit(1);
    });

    // 发起连接
    m_tcpsock->connectToHost(m_serverIp, m_port);
    // 超时处理
    if (!m_tcpsock->waitForConnected(2000))
    {
        MsgBox *box = new MsgBox(static_cast<QWidget*>(parent), "连接服务器超时! ", "错误");
        box->exec();
        exit(1);
    }
}

bool ClientWebSocket::isLogined()
{
    return m_loginStatus;
}

// 写消息
void ClientWebSocket::writeMsg(QByteArray str)
{
    // 加入魔数和数据大小头部
    uint32_t size = str.size();
    uint32_t magic = 0x17171717;
    str.prepend(size);
    str.prepend(magic);

    // 发出数据
    m_tcpsock->write(str);
}

// 读消息
QJsonObject ClientWebSocket::readMsg()
{
    QByteArray codedata;
    // 读取所有可用数据
    while (m_tcpsock->bytesAvailable() > 0) {
        codedata += m_tcpsock->readAll();
    }
    // 解码
    QByteArray data = QByteArray::fromBase64(codedata);

    // 准备解析json数据
    QJsonParseError jsonError;
    QJsonDocument jsondocu = QJsonDocument::fromJson(data, &jsonError);  // 转化为 JSON 文档
    if (jsonError.error != QJsonParseError::NoError) {
        qWarning() << "解析Json错误:" << jsonError.errorString();
        QJsonObject jsondata;
        return jsondata;
    }
    // 获取解析后的json数据
    QJsonObject jsondata = jsondocu.object();
    return jsondata;
}

// 发送登陆注册消息
void ClientWebSocket::sendLoginSignupMsg(QJsonObject msg)
{
    // 注册可读信号槽事件
    QObject::connect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::readLoginSignRetMsg);

    // 创建 QJsonDocument 对象，用于处理 JSON 数据
    QJsonDocument jsonDocument(msg);
    // 将 QJsonDocument 转换为 QByteArray
    QByteArray byteArray = jsonDocument.toJson();
    // 将 QByteArray 转换为 base64 编码
    QByteArray codeStr = byteArray.toBase64().constData();

    // 发出数据
    writeMsg(codeStr);
}

// 上传图片
void ClientWebSocket::uploadImg(QJsonObject msg)
{
    msg.insert("request", "upload");
    QJsonDocument jsonDocument(msg);
    QByteArray byteArray = jsonDocument.toJson();
    QByteArray codeStr = byteArray.toBase64().constData();

    // 发出数据
    writeMsg(codeStr);
}

// 获取云端列表
void ClientWebSocket::getCloudList(QString username)
{
    QObject::connect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::processGetListRetMsg);

    QJsonObject msg;
    msg.insert("request","getlist");
    msg.insert("username", username);
    QJsonDocument jsonDocument(msg);
    QByteArray byteArray = jsonDocument.toJson();
    QByteArray codeStr = byteArray.toBase64().constData();

    // 发出数据
    writeMsg(codeStr);
}

// 获取图片数据
void ClientWebSocket::getImg(QString username, QString imgname)
{
    QObject::connect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::processImgDataMsg);
    // 创建Json数据
    QJsonObject userdata;
    userdata.insert("username", username);
    userdata.insert("imgname", imgname);
    QJsonObject msg;
    msg.insert("userdata","userdata");

    // base64编码
    QJsonDocument jsonDocument(msg);
    QByteArray byteArray = jsonDocument.toJson();
    QByteArray codeStr = byteArray.toBase64().constData();

    // 发出数据
    writeMsg(codeStr);
}

// 处理登陆注册返回的信息
void ClientWebSocket::readLoginSignRetMsg()
{
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());

    QJsonObject jsondata = readMsg();
    if(jsondata.isEmpty())
    {
        MsgBox *box = new MsgBox(parentWidget, "json解析为空", "错误");
        box->exec();
        return;
    }

    // 读取状态和信息
    int status = jsondata.value("status").toVariant().toInt();
    QString msg = jsondata.value("msg").toString();
    if(status != 0)
    {
        MsgBox *box = new MsgBox(parentWidget, msg, "错误");
        box->exec();
        return;
    }
    MsgBox *box = new MsgBox(parentWidget, msg);
    box->exec();

    // 记录登录状态
    if(msg == "登录成功")
    {
        m_loginStatus = true;
    }

    // 取消可读回调
    QObject::disconnect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::readLoginSignRetMsg);
    return;
}

// 处理登录注册返回的信息
void ClientWebSocket::processGetListRetMsg()
{
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    // 断开信号和槽
    QObject::disconnect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::processGetListRetMsg);

    QJsonObject jsondata = readMsg();
    if(jsondata.isEmpty())
    {
        MsgBox *box = new MsgBox(parentWidget, "json解析为空", "错误");
        box->exec();
        return;
    }

    // 获取图片信息列表
    QJsonArray imgList = jsondata["imglist"].toArray();

    QStringList imgs;
    for(int i = 0; i < imgList.size(); ++i)
    {
        imgs.append(imgList.at(i).toString());
    }
    emit imgListReceived(imgs);
}

// 处理图片信息
void ClientWebSocket::processImgDataMsg()
{
    QWidget *parentWidget = qobject_cast<QWidget*>(parent());
    QObject::disconnect(m_tcpsock, &QTcpSocket::readyRead, this, &ClientWebSocket::processImgDataMsg);

    // 读取socket数据
    QJsonObject jsondata = readMsg();
    if(jsondata.isEmpty())
    {
        MsgBox *box = new MsgBox(parentWidget, "json解析为空", "错误");
        box->exec();
        return;
    }
    // 读取状态和信息
    int status = jsondata.value("status").toVariant().toInt();
    // 出错
    if(status != 0)
    {
        QString msg = jsondata.value("msg").toString();
        MsgBox *box = new MsgBox(parentWidget, msg, "错误");
        box->exec();
        return;
    }
    QString imgdata = jsondata.value("imgdata").toString();

    // aes128 解码
    QByteArray key("FileStoreService");
    QAESEncryption aesEncryption(QAESEncryption::AES_128, QAESEncryption::CBC);
    QByteArray byteArray = imgdata.toUtf8();
    QByteArray decryptedData = aesEncryption.decode(byteArray, key, key);

    // 触发接受信号
    emit imgDataReceived(decryptedData);
}

