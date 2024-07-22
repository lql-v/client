#ifndef CLIENTWEBSOCKET_H
#define CLIENTWEBSOCKET_H

#include "msgbox.h"
#include "qaesencryption.h"

#include <QTcpSocket>
#include <QObject>
#include <QDebug>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
class MainWindow;
class ClientWebSocket : public QObject
{
    Q_OBJECT
public:
    explicit ClientWebSocket(QObject *parent = nullptr);

    bool isLogined();                               // 检测登录状态

    void sendLoginSignupMsg(QJsonObject msg);       // 发送登录注册消息
    void uploadImg(QJsonObject msg);                // 上传图片
    void getCloudList(QString username);            // 获取云端列表
    void getImg(QString username, QString imgname); // 获取图片数据
signals:
    void imgListReceived(QStringList imgList);      // 接受图片列表完成
    void imgDataReceived(QByteArray imgdata);                         // 接受图片数据完成

private:
    void writeMsg(QByteArray str);                  // 写出消息辅助函数（添加自定义头部）
    QJsonObject readMsg();                          // 读取数据
private slots:
    void readLoginSignRetMsg();                     // 处理登录注册返回的信息
    void processGetListRetMsg();                    // 处理云列表信息
    void processImgDataMsg();                       // 处理图片信息
private:
    QTcpSocket *m_tcpsock;
    QString m_serverIp="127.0.0.1";
    unsigned short m_port=8080;
    bool m_loginStatus = false; // 是否已经登录
};

#endif // CLIENTWEBSOCKET_H
