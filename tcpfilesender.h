#ifndef TCPFILESENDER_H
#define TCPFILESENDER_H

#include <QDialog>
#include <QtNetwork>
#include <QtWidgets>

class TcpFileSender : public QDialog
{
    Q_OBJECT

public:
    TcpFileSender(QWidget *parent = 0);
    ~TcpFileSender();

public slots:
    void start();
    void startTransfer();
    void updateClientProgress(qint64 numBytes);
    void openFile();

private:
    QProgressBar     *clientProgressBar;
    QLabel           *clientStatusLabel;
    QPushButton      *startButton;
    QPushButton      *quitButton;
    QPushButton      *openButton;
    QDialogButtonBox *buttonBox;
    QTcpSocket       tcpClient;
    QLineEdit        *ipInput;
    QLineEdit        *portInput;

    qint64           totalBytes;
    qint64           bytesWritten;
    qint64           bytesToWrite;
    qint64           loadSize;
    QString          fileName;
    QFile            *localFile;
    QByteArray       outBlock;
};

#endif // TCPFILESENDER_H
