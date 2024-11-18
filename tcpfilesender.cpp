#include "tcpfilesender.h"

TcpFileSender::TcpFileSender(QWidget *parent)
    : QDialog(parent)
{
    loadSize = 1024 * 4;
    totalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;

    clientProgressBar = new QProgressBar;
    clientStatusLabel = new QLabel(QStringLiteral("客戶端就緒"));

    ipInput = new QLineEdit;   // 建立 IP 輸入框
    ipInput->setPlaceholderText(QStringLiteral("輸入伺服器 IP"));
    ipInput->setText("140.130.35.214"); // 預設 IP 為本機地址

    portInput = new QLineEdit; // 建立 Port 輸入框
    portInput->setPlaceholderText(QStringLiteral("輸入伺服器 Port"));
    portInput->setText("16998");   // 預設 Port 為 12345

    startButton = new QPushButton(QStringLiteral("開始"));
    quitButton = new QPushButton(QStringLiteral("退出"));
    openButton = new QPushButton(QStringLiteral("開檔"));

    startButton->setEnabled(false);

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(openButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(clientProgressBar);
    mainLayout->addWidget(clientStatusLabel);
    mainLayout->addWidget(ipInput);
    mainLayout->addWidget(portInput);
    mainLayout->addStretch(1);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    setWindowTitle(QStringLiteral("(版本控制Git管理)檔案傳送"));

    connect(openButton, &QPushButton::clicked, this, &TcpFileSender::openFile);
    connect(startButton, &QPushButton::clicked, this, &TcpFileSender::start);
    connect(&tcpClient, &QTcpSocket::connected, this, &TcpFileSender::startTransfer);
    connect(&tcpClient, &QTcpSocket::bytesWritten, this, &TcpFileSender::updateClientProgress);
    connect(quitButton, &QPushButton::clicked, this, &TcpFileSender::close);
}

void TcpFileSender::openFile()
{
    fileName = QFileDialog::getOpenFileName(this);
    if (!fileName.isEmpty())
        startButton->setEnabled(true);
}

void TcpFileSender::start()
{
    startButton->setEnabled(false);
    bytesWritten = 0;

    // 檢查是否已連接，若有則中止
    if (tcpClient.state() == QAbstractSocket::ConnectedState ||
        tcpClient.state() == QAbstractSocket::ConnectingState) {
        tcpClient.abort();
    }

    QString ipAddress = ipInput->text();
    QString portString = portInput->text();
    bool portOk;
    quint16 port = portString.toUShort(&portOk);

    if (!portOk) {
        QMessageBox::warning(this, QStringLiteral("錯誤"), QStringLiteral("無效的端口號"));
        startButton->setEnabled(true);
        return;
    }

    clientStatusLabel->setText(QStringLiteral("連接中..."));
    tcpClient.connectToHost(ipAddress, port);
}

void TcpFileSender::startTransfer()
{
    startButton->setEnabled(false);

    localFile = new QFile(fileName);
    if (!localFile->open(QFile::ReadOnly)) {
        QMessageBox::warning(this, QStringLiteral("應用程式"),
                             QStringLiteral("無法讀取 %1:\n%2.").arg(fileName).arg(localFile->errorString()));
        startButton->setEnabled(true);
        return;
    }

    totalBytes = localFile->size();
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);
    sendOut.setVersion(QDataStream::Qt_4_6);
    QString currentFile = fileName.right(fileName.size() - fileName.lastIndexOf("/") - 1);
    sendOut << qint64(0) << qint64(0) << currentFile;
    totalBytes += outBlock.size();

    sendOut.device()->seek(0);
    sendOut << totalBytes << qint64(outBlock.size() - sizeof(qint64) * 2);
    bytesToWrite = totalBytes - tcpClient.write(outBlock);

    clientStatusLabel->setText(QStringLiteral("已連接"));
    qDebug() << currentFile << totalBytes;
    outBlock.resize(0);
}

void TcpFileSender::updateClientProgress(qint64 numBytes)
{
    bytesWritten += (int)numBytes;

    if (bytesToWrite > 0) {
        outBlock = localFile->read(qMin(bytesToWrite, loadSize));
        bytesToWrite -= (int)tcpClient.write(outBlock);
        outBlock.resize(0);
    } else {
        localFile->close();
    }

    clientProgressBar->setMaximum(totalBytes);
    clientProgressBar->setValue(bytesWritten);
    clientStatusLabel->setText(QStringLiteral("已傳送 %1 Bytes").arg(bytesWritten));
}

TcpFileSender::~TcpFileSender()
{
    if (tcpClient.state() == QAbstractSocket::ConnectedState) {
        tcpClient.disconnectFromHost();
        if (tcpClient.state() != QAbstractSocket::UnconnectedState) {
            tcpClient.waitForDisconnected(3000);
        }
    }
}
