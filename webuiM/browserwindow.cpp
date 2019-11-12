#include "browser.h"
#include "browserwindow.h"
#include "downloadmanagerwidget.h"
#include "bookmarkmanagerwidget.h"
#include "tabwidget.h"
#include "webview.h"
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QEvent>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QScreen>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWebEngineProfile>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QToolTip>
#include <QSize>
#include <QPushButton>
#include <QTextEdit>
#include <QHttpMultiPart>
#include <QStringLiteral>
//n<<QString("configs/network.conf");
//n<<QString("configs/checkstatus.conf");
//n<<QString("configs/status_cvbs.conf");
//n<<QString("configs/status_cvbs2.conf");
//n<<QString("configs/status_ai");
//n<<QString("configs/status_hdmi.conf");
//n<<QString("configs/status_hdmi2.conf");
//n<<QString("configs/status_interrupt");
//n<<QString("configs/status_vi");
//n<<QString("configs/status_wifi");

#define DEFAULT_CONFIG(n) \
do{n.clear();\
n<<QString("configs/audioencode.conf");\
n<<QString("configs/auto_reboot.conf");\
n<<QString("configs/cvbscolor.conf");\
n<<QString("configs/cvbsinfo.conf");\
n<<QString("configs/cvbsinfo2.conf");\
n<<QString("configs/cvbsosd.conf");\
n<<QString("configs/hdmicolor.conf");\
n<<QString("configs/hdmiinfo.conf"); \
n<<QString("configs/hdmiinfo2.conf");\
n<<QString("configs/hdmiosd.conf");\
n<<QString("configs/quality.conf");\
n<<QString("configs/source.conf");\
n<<QString("configs/version.info");\
n<<QString("configs/web.conf");\
}while(0)


BrowserWindow::BrowserWindow(Browser *browser, QWebEngineProfile *profile, bool forDevTools)
    : m_browser(browser)
      , m_profile(profile)
      , m_tabWidget(new TabWidget(profile, this))
      , m_progressBar(nullptr)
      , m_historyBackAction(nullptr)
      , m_historyForwardAction(nullptr)
      , m_stopAction(nullptr)
      , m_reloadAction(nullptr)
      , m_stopReloadAction(nullptr)
      , m_urlLineEdit(nullptr)
      , m_favAction(nullptr)
      , m_playerProcess(nullptr)
      , m_recordProcess(nullptr)
      , m_reply(nullptr)
      , m_file(nullptr)
      , m_udprecv(nullptr)
{
    m_quit = 0;
    m_linedit_text_bak.clear();
    setAttribute(Qt::WA_DeleteOnClose, true);
    setFocusPolicy(Qt::ClickFocus);
    m_playerProcess = new QProcess(this);
    m_playerProcess->setProcessChannelMode(QProcess::MergedChannels);
    m_recordProcess = new QProcess(this);
    m_recordProcess->setProcessChannelMode(QProcess::MergedChannels);

    if (!forDevTools) {
        m_progressBar = new QProgressBar(this);

        QToolBar *toolbar = createToolBar();
        addToolBar(toolbar);
        menuBar()->addMenu(createFileMenu(m_tabWidget));
        menuBar()->addMenu(createEditMenu());
        menuBar()->addMenu(createViewMenu(toolbar));
        menuBar()->addMenu(createWindowMenu(m_tabWidget));
        menuBar()->addMenu(createHelpMenu());
    }

    QWidget *centralWidget = new QWidget(this);    
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    if (!forDevTools) {
        addToolBarBreak();
        m_progressBar->setMaximumHeight(1);
        m_progressBar->setTextVisible(false);
        m_progressBar->setStyleSheet(QStringLiteral("QProgressBar {border: 0px} QProgressBar::chunk {background-color: #da4453}"));
        layout->addWidget(m_progressBar);
    }

    layout->addWidget(m_tabWidget);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);

    connect(m_tabWidget, &TabWidget::titleChanged, this, &BrowserWindow::handleWebViewTitleChanged);
    if (!forDevTools) {
        connect(m_tabWidget, &TabWidget::linkHovered, [this](const QString& url) {
            statusBar()->showMessage(url);
        });
        connect(m_tabWidget, &TabWidget::loadProgress, this, &BrowserWindow::handleWebViewLoadProgress);
        connect(m_tabWidget, &TabWidget::webActionEnabledChanged, this, &BrowserWindow::handleWebActionEnabledChanged);
        connect(m_tabWidget, &TabWidget::urlChanged, [this](const QUrl &url) {
            m_urlLineEdit->setText(url.toDisplayString());
        });
        connect(m_tabWidget, &TabWidget::favIconChanged, m_favAction, &QAction::setIcon);
        connect(m_tabWidget, &TabWidget::devToolsRequested, this, &BrowserWindow::handleDevToolsRequested);
        connect(m_urlLineEdit, &QLineEdit::returnPressed, [this]() {
            m_tabWidget->setUrl(QUrl::fromUserInput(m_urlLineEdit->text()));
        });

        QAction *focusUrlLineEditAction = new QAction(this);
        addAction(focusUrlLineEditAction);
        focusUrlLineEditAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
        connect(focusUrlLineEditAction, &QAction::triggered, this, [this] () {
            m_urlLineEdit->setFocus(Qt::ShortcutFocusReason);
        });
    }

    handleWebViewTitleChanged(QString());
    m_tabWidget->createTab();

}

QSize BrowserWindow::sizeHint() const
{
    QRect desktopRect = QApplication::primaryScreen()->geometry();
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

QMenu *BrowserWindow::createFileMenu(TabWidget *tabWidget)
{
    QMenu *fileMenu = new QMenu(tr("&File"));
    fileMenu->addAction(tr("&New Window"), this, &BrowserWindow::handleNewWindowTriggered, QKeySequence::New);
    fileMenu->addAction(tr("New &Incognito Window"), this, &BrowserWindow::handleNewIncognitoWindowTriggered);

    QAction *newTabAction = new QAction(tr("New &Tab"), this);
    newTabAction->setShortcuts(QKeySequence::AddTab);
    connect(newTabAction, &QAction::triggered, this, [this]() {
        m_tabWidget->createTab();
        m_urlLineEdit->setFocus();
    });
    fileMenu->addAction(newTabAction);

    fileMenu->addAction(tr("&Open File..."), this, &BrowserWindow::handleFileOpenTriggered, QKeySequence::Open);
    fileMenu->addSeparator();

    QAction *urlAction = new QAction(tr("&Open URL"),this);
    urlAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    connect(urlAction, &QAction::triggered, [=]() {
        QDialog dlg(this);
        dlg.resize(399,148);
        dlg.setWindowFlags(dlg.windowFlags()&~Qt::WindowContextHelpButtonHint);
        dlg.setWindowTitle(tr("Open URL"));
        QGridLayout glayout;
        QLabel lab(tr("Input url:"),this);
        glayout.addWidget(&lab,0,0,1,1);
        QLineEdit lin_url(m_linedit_text_bak,this);
        glayout.addWidget(&lin_url,0,1,1,9);
        QDialogButtonBox dlgbtnbox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,this);
        connect(&dlgbtnbox, &QDialogButtonBox::accepted,&dlg,&QDialog::accept);
        connect(&dlgbtnbox, &QDialogButtonBox::rejected,&dlg,&QDialog::reject);
        glayout.addWidget(&dlgbtnbox,1,0,1,10);
        dlg.setLayout(&glayout);
        if (dlg.exec() == QDialog::Accepted) {
#if defined(Q_OS_WIN)
            QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay.exe";
#else
                QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay";
#endif
            if(!lin_url.text().isEmpty()){
                QStringList args;
                QString qstr_url = lin_url.text().replace(QRegExp("[\\s]+"), " ");
                args = qstr_url.split(" ");
                if(m_playerProcess->isOpen()){
                    m_playerProcess->close();
                    m_playerProcess->waitForFinished();
                }
                m_playerProcess->start(playerpath,args);
                if(!m_playerProcess->waitForStarted()){
                    QMessageBox::warning(this, "warning", QString("player run failed!"));
                }
                m_linedit_text_bak = lin_url.text();
            }
        }
    });

    fileMenu->addAction(urlAction);

    QAction *closeTabAction = new QAction(tr("&Close Tab"), this);
    closeTabAction->setShortcuts(QKeySequence::Close);
    connect(closeTabAction, &QAction::triggered, [tabWidget]() {
        tabWidget->closeTab(tabWidget->currentIndex());
    });
    fileMenu->addAction(closeTabAction);

    QAction *closeAction = new QAction(tr("&Quit"),this);
    closeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(closeAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(closeAction);
    connect(fileMenu, &QMenu::aboutToShow, [this, closeAction]() {
        if (m_browser->windows().count() == 1)
            closeAction->setText(tr("&Quit"));
        else
            closeAction->setText(tr("&Close Window"));
    });

    return fileMenu;
}


QMenu *BrowserWindow::createEditMenu()
{
    QMenu *editMenu = new QMenu(tr("&Edit"));
    QAction *findAction = editMenu->addAction(tr("&Find"));
    findAction->setShortcuts(QKeySequence::Find);
    connect(findAction, &QAction::triggered, this, &BrowserWindow::handleFindActionTriggered);

    QAction *findNextAction = editMenu->addAction(tr("Find &Next"));
    findNextAction->setShortcut(QKeySequence::FindNext);
    connect(findNextAction, &QAction::triggered, [this]() {
        if (!currentTab() || m_lastSearch.isEmpty())
            return;
        currentTab()->findText(m_lastSearch);
    });

    QAction *findPreviousAction = editMenu->addAction(tr("Find &Previous"));
    findPreviousAction->setShortcut(QKeySequence::FindPrevious);
    connect(findPreviousAction, &QAction::triggered, [this]() {
        if (!currentTab() || m_lastSearch.isEmpty())
            return;
        currentTab()->findText(m_lastSearch, QWebEnginePage::FindBackward);
    });

    QAction *bookmarkAction = new QAction(tr("Edit &Bookmark"),this);
    bookmarkAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(bookmarkAction, &QAction::triggered, [=]() {
        QDialog dlg(this);
        dlg.resize(400,300);
        dlg.setWindowFlags(dlg.windowFlags()&~Qt::WindowContextHelpButtonHint);
        dlg.setWindowTitle(tr("Edit Bookmark"));
        QStringList iplsttex = m_browser->bookmarkManagerWidget().iniIpStringList();
        QString mptex =  m_browser->bookmarkManagerWidget().
                        iniMainPageString().section('@',1,1).
                        trimmed().section('/',0,0).trimmed();
        QGridLayout glayout;
        QPushButton btn(tr("Automatic Capture"),this);
        glayout.addWidget(&btn,0,0,1,4);
        QLabel mplab(tr("MainPage IP:"),this);
        glayout.addWidget(&mplab,1,0,1,1);
        QLineEdit *plin = new QLineEdit(mptex,this);
        glayout.addWidget(plin,1,1,1,3);
        QLabel lab(tr("Multiple Devcie IP:"),this);
        glayout.addWidget(&lab,2,0,1,1);
        QTextEdit *tex = new QTextEdit(this);
        tex->clear();
        for(int i = 0;i < iplsttex.size();i++){
            tex->insertPlainText(iplsttex.at(i).section('@',1,1).trimmed().
                                 section('/',0,0).trimmed()+QString(";\n"));
        }
        glayout.addWidget(tex,3,0,3,4);
        QDialogButtonBox dlgbtnbox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,this);
        connect(&dlgbtnbox, &QDialogButtonBox::accepted,&dlg,&QDialog::accept);
        connect(&dlgbtnbox, &QDialogButtonBox::rejected,&dlg,&QDialog::reject);
        glayout.addWidget(&dlgbtnbox,6,0,1,4);
        dlg.setLayout(&glayout);
        //udp recv
        m_udprecvsl.clear();
        if(m_udprecv != nullptr){
            delete m_udprecv;
            m_udprecv = nullptr;
        }
        m_udprecv  = new QUdpSocket(this);
        //QHostAddress address("192.168.8.100");
        // m_udprecv->bind(address,5440,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        m_udprecv->bind(5440, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
        connect(m_udprecv,&QUdpSocket::readyRead,[=](){
            qint64 pds = 0;
            while (m_udprecv->hasPendingDatagrams()) {
                QByteArray datagram;
                pds = m_udprecv->pendingDatagramSize();
                if(pds <=0){
                    qDebug()<< "udp recv erro";
                    break;
                }
                datagram.resize(pds);
                m_udprecv->readDatagram(datagram.data(),datagram.size());
                if( -1 != datagram.indexOf("TBSWEBIP:")){
                    QString datagramqstr = datagram;
                    m_udprecvsl << datagramqstr.section(':',1,1).trimmed();
                }
                qDebug()<< datagram;
            }

            if(tex != nullptr){
                tex->clear();
                for (int i = 0; i < m_udprecvsl.size(); i++){
                    for (int k = i + 1; k <  m_udprecvsl.size(); k++){
                        if ( m_udprecvsl.at(i) ==  m_udprecvsl.at(k)){
                            m_udprecvsl.removeAt(k);
                            k--;
                        }
                    }
                    tex->insertPlainText(m_udprecvsl.at(i)+QString(";\n"));
                }

                plin->setText(m_udprecvsl.at(0));
            }

        });

        connect(&btn, &QPushButton::clicked, [=]() {
            m_udprecvsl.clear();
            tex->clear();
            QUdpSocket udpsend(this);
            udpsend.writeDatagram(QByteArray("getip"),QHostAddress::Broadcast,9999);
        });

        if (dlg.exec() == QDialog::Accepted) {
            QString get_mp = QString("http://root:root@")+plin->text()+"/";
            QStringList get_texlist = tex->toPlainText().split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
            QString get_tex = get_texlist.join(",");
            get_tex.remove(',');
            get_tex.remove(QRegExp("\\s"));
            QStringList get_iplst_tex = get_tex.split(";");
            QStringList get_iplst;
            get_iplst.clear();
            for(int i = 0;i< get_iplst_tex.size();i++){
                if(!get_iplst_tex.at(i).isEmpty()){
                    get_iplst << QString("http://root:root@")+get_iplst_tex.at(i)+"/";
                }
            }
            qDebug()<<get_iplst;
            m_browser->bookmarkManagerWidget().writeIni(get_mp,get_iplst);
            m_browser->bookmarkManagerWidget().updateUI();
            m_tabWidget->setUrl(get_mp);

        }

        if(nullptr != plin){
            delete plin;
            plin = nullptr;
        }

        if(nullptr != tex){
            delete tex;
            tex = nullptr;
        }

        if(m_udprecv != nullptr){
            delete m_udprecv;
            m_udprecv = nullptr;
        }
    });

    editMenu->addAction(bookmarkAction);

    /*********************play*video*******************************/
    QMenu *pChildMenu = new QMenu(this);
    pChildMenu->setTitle(QStringLiteral("Play &Video"));
    QAction *playHdmiMainAction = pChildMenu->addAction(QStringLiteral("HDMI Main"));
    playHdmiMainAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    connect(playHdmiMainAction, &QAction::triggered, [this]() {
        makeDirPath(QApplication::applicationDirPath()+"/configs/");
        QStringList filelist;
        filelist<<"configs/status_hdmi.conf";
        filelist<<"configs/hdmiinfo.conf";

        for(int i = 0; i < filelist.size();i++){
            QString qstr_urlLine = m_urlLineEdit->text();
            QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
            QUrl url(QString("http://root:root@")+qstr_url+"/"+filelist.at(i));
            qDebug()<<url;
            m_file = new QFile(QApplication::applicationDirPath() + "/"+filelist.at(i),this);
            m_file->open(QIODevice::WriteOnly);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            QNetworkAccessManager accessManager(this);
            m_reply = accessManager.get(request);
            connect((QObject *)m_reply, SIGNAL(readyRead()), this, SLOT(readContent()));
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
            connect((QObject *)m_reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(loadError(QNetworkReply::NetworkError)));
            connect((QObject *)m_reply, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(loadProgress(qint64 ,qint64)));
            QEventLoop eventLoop;
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            QMSLEEP(100);
        }

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/status_hdmi.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString data = m_file->readAll();
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/hdmiinfo.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString qstr_rtsp_transport;
        qstr_rtsp_transport.clear();
        while (!m_file->atEnd()){
            QByteArray line = m_file->readLine();
            QString str(line);
            if(-1 != str.indexOf("tcp")){
                if(0 == str.section(':',1,1).trimmed().toInt()){
                    qstr_rtsp_transport = QString("udp");
                }else {
                    qstr_rtsp_transport = QString("tcp");
                }
                break;
            }
        }
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;

#if defined(Q_OS_WIN)
        QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay.exe";
#else
            QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay";
#endif
        QStringList args;
        if(-1 != data.section(':',1).trimmed().section('\n',0,0).trimmed().indexOf("rtsp://")){
            args << "-rtsp_transport"<<qstr_rtsp_transport;
        }
        args <<"-i"<<(data.section(':',1).trimmed().section('\n',0,0).trimmed());
        qDebug()<<args;

        if(m_playerProcess->isOpen()){
            m_playerProcess->close();
            m_playerProcess->waitForFinished();
        }
        m_playerProcess->start(playerpath,args);
        if(!m_playerProcess->waitForStarted()){
            QMessageBox::warning(this, "warning", QString("Player run failed!"));
        }
    });

    QAction *playHdmi2ndAction = pChildMenu->addAction(QStringLiteral("HDMI 2nd"));
    playHdmi2ndAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));
    connect(playHdmi2ndAction, &QAction::triggered, [this]() {
        makeDirPath(QApplication::applicationDirPath()+"/configs/");
        QStringList filelist;
        filelist<<"configs/status_hdmi2.conf";
        filelist<<"configs/hdmiinfo2.conf";

        for(int i = 0; i < filelist.size();i++){
            QString qstr_urlLine = m_urlLineEdit->text();
            QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
            QUrl url(QString("http://root:root@")+qstr_url+"/"+filelist.at(i));
            qDebug()<<url;
            m_file = new QFile(QApplication::applicationDirPath() + "/"+filelist.at(i),this);
            m_file->open(QIODevice::WriteOnly);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            QNetworkAccessManager accessManager(this);
            m_reply = accessManager.get(request);
            connect((QObject *)m_reply, SIGNAL(readyRead()), this, SLOT(readContent()));
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
            connect((QObject *)m_reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(loadError(QNetworkReply::NetworkError)));
            connect((QObject *)m_reply, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(loadProgress(qint64 ,qint64)));
            QEventLoop eventLoop;
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            QMSLEEP(100);
        }

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/status_hdmi2.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString data = m_file->readAll();
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;


        m_file = new QFile(QApplication::applicationDirPath() + "/configs/hdmiinfo2.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString qstr_rtsp_transport;
        qstr_rtsp_transport.clear();
        while (!m_file->atEnd()){
            QByteArray line = m_file->readLine();
            QString str(line);
            if(-1 != str.indexOf("tcp")){
                if(0 == str.section(':',1,1).trimmed().toInt()){
                    qstr_rtsp_transport = QString("udp");
                }else {
                    qstr_rtsp_transport = QString("tcp");
                }
                break;
            }
        }
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;

#if defined(Q_OS_WIN)
        QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay.exe";
#else
            QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay";
#endif
        QStringList args;
        if(-1 != data.section(':',1).trimmed().section('\n',0,0).trimmed().indexOf("rtsp://")){
            args << "-rtsp_transport"<<qstr_rtsp_transport;
        }
        args <<"-i"<<(data.section(':',1).trimmed().section('\n',0,0).trimmed());
        qDebug()<<args;

        if(m_playerProcess->isOpen()){
            m_playerProcess->close();
            m_playerProcess->waitForFinished();
        }
        m_playerProcess->start(playerpath,args);
        if(!m_playerProcess->waitForStarted()){
            QMessageBox::warning(this, "warning", QString("Player run failed!"));
        }
    });
    editMenu->addMenu(pChildMenu);
    /**********************end play video************************************************************/

    /********************record video**********************************************************************/
    QMenu *pChildRecordMenu = new QMenu(this);
    pChildRecordMenu->setTitle(QStringLiteral("Record &Video"));
    QAction *recordHdmiMainAction = pChildRecordMenu->addAction(QStringLiteral("HDMI Main"));
    recordHdmiMainAction->setShortcut(QKeySequence(Qt::CTRL |  Qt::Key_3));
    connect(recordHdmiMainAction, &QAction::triggered, [this]() {
        makeDirPath(QApplication::applicationDirPath()+"/configs/");
        QStringList filelist;
        filelist<<"configs/status_hdmi.conf";
        filelist<<"configs/hdmiinfo.conf";
        for(int i = 0; i < filelist.size();i++){
            QString qstr_urlLine = m_urlLineEdit->text();
            QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
            QUrl url(QString("http://root:root@")+qstr_url+"/"+filelist.at(i));
            qDebug()<<url;
            m_file = new QFile(QApplication::applicationDirPath() + "/"+filelist.at(i),this);
            m_file->open(QIODevice::WriteOnly);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            QNetworkAccessManager accessManager(this);
            m_reply = accessManager.get(request);
            connect((QObject *)m_reply, SIGNAL(readyRead()), this, SLOT(readContent()));
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
            connect((QObject *)m_reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(loadError(QNetworkReply::NetworkError)));
            connect((QObject *)m_reply, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(loadProgress(qint64 ,qint64)));
            QEventLoop eventLoop;
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            QMSLEEP(100);
        }

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/status_hdmi.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString data = m_file->readAll();
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/hdmiinfo.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString qstr_rtsp_transport;
        qstr_rtsp_transport.clear();
        while (!m_file->atEnd()){
            QByteArray line = m_file->readLine();
            QString str(line);
            if(-1 != str.indexOf("tcp")){
                if(0 == str.section(':',1,1).trimmed().toInt()){
                    qstr_rtsp_transport = QString("udp");
                }else {
                    qstr_rtsp_transport = QString("tcp");
                }
                break;
            }
        }
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;

#if defined(Q_OS_WIN)
        QString recordpath = QCoreApplication::applicationDirPath()+"/ffmpeg.exe";
#else
            QString playerpath = QCoreApplication::applicationDirPath()+"/ffmpeg";
#endif
        QStringList args;
        QTime t=QTime::currentTime();
        if(-1 != data.section(':',1).trimmed().section('\n',0,0).trimmed().indexOf("rtsp://")){
            args << "-rtsp_transport"<<qstr_rtsp_transport;
        }
        args <<"-i"<<(data.section(':',1).trimmed().section('\n',0,0).trimmed())
             << "-map" <<"?:p:?"<<"-c"<<"copy"<<"-y"<<"-f"<<"mpegts"
             <<"-fflags"<<"autobsf"<< QString("hdmiMain_")+t.toString("HH_mm_ss")+".ts";
        qDebug()<<args;
        if(m_recordProcess->isOpen()){
            m_recordProcess->close();
            m_recordProcess->waitForFinished();
        }
        m_recordProcess->start(recordpath,args);
        if(!m_recordProcess->waitForStarted()){
            QMessageBox::warning(this, "warning", QString("Record run failed!"));
        }
    });

    QAction *recordHdmi2ndAction = pChildRecordMenu->addAction(QStringLiteral("HDMI 2nd"));
    recordHdmi2ndAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_4));
    connect(recordHdmi2ndAction, &QAction::triggered, [this]() {
        makeDirPath(QApplication::applicationDirPath()+"/configs/");
        QStringList filelist;
        filelist<<"configs/status_hdmi2.conf";
        filelist<<"configs/hdmiinfo2.conf";

        for(int i = 0; i < filelist.size();i++){
            QString qstr_urlLine = m_urlLineEdit->text();
            QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
            QUrl url(QString("http://root:root@")+qstr_url+"/"+filelist.at(i));
            qDebug()<<url;
            m_file = new QFile(QApplication::applicationDirPath() + "/"+filelist.at(i),this);
            m_file->open(QIODevice::WriteOnly);
            QNetworkRequest request(url);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            QNetworkAccessManager accessManager(this);
            m_reply = accessManager.get(request);
            connect((QObject *)m_reply, SIGNAL(readyRead()), this, SLOT(readContent()));
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
            connect((QObject *)m_reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(loadError(QNetworkReply::NetworkError)));
            connect((QObject *)m_reply, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(loadProgress(qint64 ,qint64)));
            QEventLoop eventLoop;
            connect(&accessManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
            eventLoop.exec();
            QMSLEEP(100);
        }

        m_file = new QFile(QApplication::applicationDirPath() + "/configs/status_hdmi2.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString data = m_file->readAll();
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;


        m_file = new QFile(QApplication::applicationDirPath() + "/configs/hdmiinfo2.conf",this);
        m_file->open(QIODevice::ReadOnly);
        QString qstr_rtsp_transport;
        qstr_rtsp_transport.clear();
        while (!m_file->atEnd()){
            QByteArray line = m_file->readLine();
            QString str(line);
            if(-1 != str.indexOf("tcp")){
                if(0 == str.section(':',1,1).trimmed().toInt()){
                    qstr_rtsp_transport = QString("udp");
                }else {
                    qstr_rtsp_transport = QString("tcp");
                }
                break;
            }
        }
        m_file->close();
        m_file->deleteLater();
        m_file = nullptr;
#if defined(Q_OS_WIN)
        QString recordpath = QCoreApplication::applicationDirPath()+"/ffmpeg.exe";
#else
            QString playerpath = QCoreApplication::applicationDirPath()+"/ffmpeg";
#endif
        QStringList args;
        QTime t=QTime::currentTime();
        if(-1 != data.section(':',1).trimmed().section('\n',0,0).trimmed().indexOf("rtsp://")){
            args << "-rtsp_transport"<<qstr_rtsp_transport;
        }
        args <<"-i"<<(data.section(':',1).trimmed().section('\n',0,0).trimmed())
             << "-map" <<"?:p:?"<<"-c"<<"copy"<<"-y"<<"-f"<<"mpegts"
             <<"-fflags"<<"autobsf"<< QString("hdmiMain_")+t.toString("HH_mm_ss")+".ts";
        qDebug()<<args;
        if(m_recordProcess->isOpen()){
            m_recordProcess->close();
            m_recordProcess->waitForFinished();
        }

        m_recordProcess->start(recordpath,args);
        if(!m_recordProcess->waitForStarted()){
            QMessageBox::warning(this, "warning", QString("Record run failed!"));
        }
    });
    editMenu->addMenu(pChildRecordMenu);
    /*********************end record video**********************************************************************/
    QAction *stopRecordAction = editMenu->addAction(tr("Record &Stop"));
    stopRecordAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_5));
    connect(stopRecordAction, &QAction::triggered, [this]() {
        if(m_recordProcess->isOpen()){
            m_recordProcess->close();
            m_recordProcess->waitForFinished();
        }
    });

    QAction *backupAction = editMenu->addAction(tr("Backup &files"));
    backupAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(backupAction, &QAction::triggered, [this]() {
        //deleteDirectory(QApplication::applicationDirPath()+"/configs/");
        makeDirPath(QApplication::applicationDirPath()+"/configs/");
        QStringList filelist;
        DEFAULT_CONFIG(filelist);
        m_quit = 0;
        int i = 0;
        for(i = 0; i < filelist.size();i++){
            if(1 == m_quit){
                m_quit = 0;
                QString str("Backup files failed!!!");
                str.resize(40,' ');
                QMessageBox::critical(this, tr("Error"),str);
                break;
            }
            if((nullptr == m_reply) && (nullptr == m_file)){
                //http://root@192.168.1.168/
                QString qstr_urlLine = m_urlLineEdit->text();
                QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
                QUrl url(QString("http://root:root@")+qstr_url+"/"+filelist.at(i));
                qDebug()<<url;
                m_file = new QFile(QApplication::applicationDirPath() + "/"+filelist.at(i),this);
                m_file->open(QIODevice::WriteOnly);
                QNetworkRequest request(url);
                request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
                QNetworkAccessManager accessManager(this);
                m_reply = accessManager.get(request);
                connect((QObject *)m_reply, SIGNAL(readyRead()), this, SLOT(readContent()));
                connect(&accessManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));
                connect((QObject *)m_reply, SIGNAL(error(QNetworkReply::NetworkError)),this,SLOT(loadError(QNetworkReply::NetworkError)));
                connect((QObject *)m_reply, SIGNAL(downloadProgress(qint64 ,qint64)), this, SLOT(loadProgress(qint64 ,qint64)));
                QEventLoop eventLoop;
                connect(&accessManager, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));
                eventLoop.exec();
                QMSLEEP(1);
            }else{
                QString str("Backup files failed!!!");
                str.resize(40,' ');
                QMessageBox::critical(this, tr("Error"),str);
                break;
            }
        }
        if(i == filelist.size()){
            QString str("Backup files success!!!");
            str.resize(40,' ');
            QMessageBox::information(this, tr("Success"),str);
        }
    });

    QAction *uploadAction = editMenu->addAction(tr("Upload &files"));
    uploadAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_U));
    connect(uploadAction, &QAction::triggered, [=]() {
        QDir configsdir(QApplication::applicationDirPath()+"/configs/");
        if(configsdir.exists()){
            QStringList filelist;
            DEFAULT_CONFIG(filelist);
            int i = 0;
            for(i = 0; i < filelist.size();i++){             
                QString qstr_urlLine = m_urlLineEdit->text();
                QString qstr_url = qstr_urlLine.section('@',1,1).trimmed().section('/',0,0).trimmed();
                QString apppath = QString(QApplication::applicationDirPath());

#if defined(Q_OS_WIN)
                QString curlth =  apppath+"/curl.exe";
#else
                    QString curlth =  apppath+"/curl";
#endif
                QString confpath =  QString("file=@"+apppath+"/"+filelist.at(i));
                QString httpurl = QString("http://root:root@")+qstr_url+"/cgi-bin/conf_upload.cgi";
                QStringList argls;
                argls.append("-F");
                argls.append(confpath);
                argls.append(httpurl);
                qDebug() << argls;
                QProcess::execute(curlth,argls);
            }

            QString str("Upload files success!!!");
            str.resize(40,' ');
            QMessageBox::information(this, tr("Success"),str);
        }else{
            QString str("Upload files not exists!!!");
            str.resize(40,' ');
            QMessageBox::critical(this, tr("Error"),str);
        }
    });

    return editMenu;
}

QMenu *BrowserWindow::createViewMenu(QToolBar *toolbar)
{
    QMenu *viewMenu = new QMenu(tr("&View"));
    m_stopAction = viewMenu->addAction(tr("&Stop"));
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_stopAction->setShortcuts(shortcuts);
    connect(m_stopAction, &QAction::triggered, [this]() {
        m_tabWidget->triggerWebPageAction(QWebEnginePage::Stop);
    });

    m_reloadAction = viewMenu->addAction(tr("Reload Page"));
    m_reloadAction->setShortcuts(QKeySequence::Refresh);
    connect(m_reloadAction, &QAction::triggered, [this]() {
        m_tabWidget->triggerWebPageAction(QWebEnginePage::Reload);
    });

    QAction *zoomIn = viewMenu->addAction(tr("Zoom &In"));
    zoomIn->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    connect(zoomIn, &QAction::triggered, [this]() {
        if (currentTab())
            currentTab()->setZoomFactor(currentTab()->zoomFactor() + 0.1);
    });

    QAction *zoomOut = viewMenu->addAction(tr("Zoom &Out"));
    zoomOut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    connect(zoomOut, &QAction::triggered, [this]() {
        if (currentTab())
            currentTab()->setZoomFactor(currentTab()->zoomFactor() - 0.1);
    });

    QAction *resetZoom = viewMenu->addAction(tr("Reset &Zoom"));
    resetZoom->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(resetZoom, &QAction::triggered, [this]() {
        if (currentTab())
            currentTab()->setZoomFactor(1.0);
    });


    viewMenu->addSeparator();
    QAction *viewToolbarAction = new QAction(tr("Hide Toolbar"),this);
    viewToolbarAction->setShortcut(tr("Ctrl+|"));
    connect(viewToolbarAction, &QAction::triggered, [toolbar,viewToolbarAction]() {
        if (toolbar->isVisible()) {
            viewToolbarAction->setText(tr("Show Toolbar"));
            toolbar->close();
        } else {
            viewToolbarAction->setText(tr("Hide Toolbar"));
            toolbar->show();
        }
    });
    viewToolbarAction->setText(tr("Show Toolbar"));
    toolbar->close();
    viewMenu->addAction(viewToolbarAction);

    QAction *viewStatusbarAction = new QAction(tr("Hide Status Bar"), this);
    viewStatusbarAction->setShortcut(tr("Ctrl+/"));
    connect(viewStatusbarAction, &QAction::triggered, [this, viewStatusbarAction]() {
        if (statusBar()->isVisible()) {
            viewStatusbarAction->setText(tr("Show Status Bar"));
            statusBar()->close();
        } else {
            viewStatusbarAction->setText(tr("Hide Status Bar"));
            statusBar()->show();
        }
    });
    viewStatusbarAction->setText(tr("Show Status Bar"));
    statusBar()->close();
    viewMenu->addAction(viewStatusbarAction);

    QAction *viewBookmarkAction = new QAction(tr("Hide Bookmark"), this);
    m_browser->bookmarkManagerWidget().doQObject(viewBookmarkAction);
    viewBookmarkAction->setShortcut(tr("Ctrl+B"));
    connect(viewBookmarkAction, &QAction::triggered, [this]() {
        if (m_browser->bookmarkManagerWidget().isVisible()) {
            m_browser->bookmarkManagerWidget().close();
        } else {
            m_browser->bookmarkManagerWidget().show();
            m_browser->bookmarkManagerWidget().setWindowOpacityAll(0.9);
        }
        m_browser->bookmarkManagerWidget().resize(m_tabWidget->width()/10,m_tabWidget->height()*1/3);
        m_browser->bookmarkManagerWidget().move(QCursor::pos());
    });

    viewBookmarkAction->setText(tr("Hide Bookmark"));
    m_browser->bookmarkManagerWidget().show();
    m_browser->bookmarkManagerWidget().setWindowOpacityAll(0.9);

    viewMenu->addAction(viewBookmarkAction);

    connect(&m_browser->bookmarkManagerWidget(), &BookmarkManagerWidget::sig_listWidget_clicked, [=](QUrl url) {
        qDebug()<< url;
        m_tabWidget->setUrl(url);
    });

    return viewMenu;
}

QMenu *BrowserWindow::createWindowMenu(TabWidget *tabWidget)
{
    QMenu *menu = new QMenu(tr("&Window"));

    QAction *nextTabAction = new QAction(tr("Show Next Tab"), this);
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BraceRight));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BracketRight));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Less));
    nextTabAction->setShortcuts(shortcuts);
    connect(nextTabAction, &QAction::triggered, tabWidget, &TabWidget::nextTab);

    QAction *previousTabAction = new QAction(tr("Show Previous Tab"), this);
    shortcuts.clear();
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BraceLeft));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_PageUp));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Greater));
    previousTabAction->setShortcuts(shortcuts);
    connect(previousTabAction, &QAction::triggered, tabWidget, &TabWidget::previousTab);

    connect(menu, &QMenu::aboutToShow, [this, menu, nextTabAction, previousTabAction]() {
        menu->clear();
        menu->addAction(nextTabAction);
        menu->addAction(previousTabAction);
        menu->addSeparator();

        QVector<BrowserWindow*> windows = m_browser->windows();
        int index(-1);
        for (auto window : windows) {
            QAction *action = menu->addAction(window->windowTitle(), this, &BrowserWindow::handleShowWindowTriggered);
            action->setData(++index);
            action->setCheckable(true);
            if (window == this)
                action->setChecked(true);
        }
    });
    return menu;
}

QMenu *BrowserWindow::createHelpMenu()
{
    QMenu *helpMenu = new QMenu(tr("&Help"));
    // helpMenu->addAction(tr("About &webM"), &m_tabWidget,setUrl(QUrl("https://www.tbsdtv.com")));

    QAction *aboutAction = new QAction(tr("About &WebuiM"), this);
    aboutAction->setShortcut(tr("Ctrl+A"));
    connect(aboutAction, &QAction::triggered, [this]() {
        QString dlgTitle="About WebuiM";
        QString strInfo="From revision 1.0 \nCopyright 2019-2025 The TBS Company Ltd. All rights reserved\n";
        QMessageBox::about(this, dlgTitle, strInfo);
    });
    helpMenu->addAction(aboutAction);

    QAction *officialAction = new QAction(tr("Official &Website"), this);
    connect(officialAction, &QAction::triggered, [this]() {
        this->m_tabWidget->setUrl(QUrl(QStringLiteral("https://www.tbsdtv.com")));
    });
    helpMenu->addAction(officialAction);

    QAction *downloadAction = new QAction(tr("Download &Website"), this);
    connect(downloadAction, &QAction::triggered, [this]() {
        this->m_tabWidget->setUrl(QUrl(QStringLiteral("https://www.tbsdtv.com/download/")));
    });
    helpMenu->addAction(downloadAction);

    return helpMenu;
}

QToolBar *BrowserWindow::createToolBar()
{
    QToolBar *navigationBar = new QToolBar(tr("Navigation"));
    navigationBar->setMovable(false);
    navigationBar->toggleViewAction()->setEnabled(false);

    m_historyBackAction = new QAction(this);
    QList<QKeySequence> backShortcuts = QKeySequence::keyBindings(QKeySequence::Back);
    for (auto it = backShortcuts.begin(); it != backShortcuts.end();) {
        // Chromium already handles navigate on backspace when appropriate.
        if ((*it)[0] == Qt::Key_Backspace)
            it = backShortcuts.erase(it);
        else
            ++it;
    }
    // For some reason Qt doesn't bind the dedicated Back key to Back.
    backShortcuts.append(QKeySequence(Qt::Key_Back));
    m_historyBackAction->setShortcuts(backShortcuts);
    m_historyBackAction->setIconVisibleInMenu(false);
    m_historyBackAction->setIcon(QIcon(QStringLiteral(":go-previous.png")));
    m_historyBackAction->setToolTip(tr("Go back in history"));
    connect(m_historyBackAction, &QAction::triggered, [this]() {
        m_tabWidget->triggerWebPageAction(QWebEnginePage::Back);
    });
    navigationBar->addAction(m_historyBackAction);

    m_historyForwardAction = new QAction(this);
    QList<QKeySequence> fwdShortcuts = QKeySequence::keyBindings(QKeySequence::Forward);
    for (auto it = fwdShortcuts.begin(); it != fwdShortcuts.end();) {
        if (((*it)[0] & Qt::Key_unknown) == Qt::Key_Backspace)
            it = fwdShortcuts.erase(it);
        else
            ++it;
    }
    fwdShortcuts.append(QKeySequence(Qt::Key_Forward));
    m_historyForwardAction->setShortcuts(fwdShortcuts);
    m_historyForwardAction->setIconVisibleInMenu(false);
    m_historyForwardAction->setIcon(QIcon(QStringLiteral(":go-next.png")));
    m_historyForwardAction->setToolTip(tr("Go forward in history"));
    connect(m_historyForwardAction, &QAction::triggered, [this]() {
        m_tabWidget->triggerWebPageAction(QWebEnginePage::Forward);
    });
    navigationBar->addAction(m_historyForwardAction);

    m_stopReloadAction = new QAction(this);
    connect(m_stopReloadAction, &QAction::triggered, [this]() {
        m_tabWidget->triggerWebPageAction(QWebEnginePage::WebAction(m_stopReloadAction->data().toInt()));
    });
    navigationBar->addAction(m_stopReloadAction);

    m_urlLineEdit = new QLineEdit(this);
    m_favAction = new QAction(this);
    m_urlLineEdit->addAction(m_favAction, QLineEdit::LeadingPosition);
    m_urlLineEdit->setClearButtonEnabled(true);
    navigationBar->addWidget(m_urlLineEdit);

    auto downloadsAction = new QAction(this);
    downloadsAction->setIcon(QIcon(QStringLiteral(":go-bottom.png")));
    downloadsAction->setToolTip(tr("Show downloads"));
    navigationBar->addAction(downloadsAction);
    connect(downloadsAction, &QAction::triggered, [this]() {
        m_browser->downloadManagerWidget().show();
    });

    return navigationBar;
}

bool BrowserWindow::makeDirPath(QString dirPath)
{
    QString dest=dirPath;
    QDir dir;
    if(!dir.exists(dest)){
        dir.mkpath(dest);
        return true;
    }else{
        return false;
    }
}

bool BrowserWindow::deleteDirectory(const QString &path)
{
    if (path.isEmpty())
        return false;

    QDir dir(path);
    if(!dir.exists())
        return true;

    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    QFileInfoList fileList = dir.entryInfoList();
    foreach (QFileInfo fi, fileList){
        if (fi.isFile())
            fi.dir().remove(fi.fileName());
        else
            deleteDirectory(fi.absoluteFilePath());
    }
    return dir.rmpath(dir.absolutePath());
}

void BrowserWindow::handleWebActionEnabledChanged(QWebEnginePage::WebAction action, bool enabled)
{
    switch (action) {
    case QWebEnginePage::Back:
        m_historyBackAction->setEnabled(enabled);
        break;
    case QWebEnginePage::Forward:
        m_historyForwardAction->setEnabled(enabled);
        break;
    case QWebEnginePage::Reload:
        m_reloadAction->setEnabled(enabled);
        break;
    case QWebEnginePage::Stop:
        m_stopAction->setEnabled(enabled);
        break;
    default:
        qWarning("Unhandled webActionChanged signal");
    }
}

void BrowserWindow::handleWebViewTitleChanged(const QString &title)
{
    QString suffix = m_profile->isOffTheRecord()
                         ? tr("webuiM (Incognito)")
                         : tr("webuiM");

    if (title.isEmpty())
        setWindowTitle(suffix);
    else
        setWindowTitle(title + " - " + suffix);
}

void BrowserWindow::handleNewWindowTriggered()
{
    BrowserWindow *window = m_browser->createWindow();
    window->m_urlLineEdit->setFocus();
}

void BrowserWindow::handleNewIncognitoWindowTriggered()
{
    BrowserWindow *window = m_browser->createWindow(/* offTheRecord: */ true);
    window->m_urlLineEdit->setFocus();
}

void BrowserWindow::handleFileOpenTriggered()
{
    QUrl url = QFileDialog::getOpenFileUrl(this, tr("Open Web Resource"), QString(),
                                           tr("Web Resources (*.html *.htm *.svg *.png *.gif *.svgz *.pdf *.txt);;"
                                              "Video files (*.ts *.mp4 *.mov *.avi *.rmvb *.rm *.flv *.3gp);;"
                                              "All files (*.*)"));
    if (url.isEmpty())
        return;
    QString localfile = url.toLocalFile();
    qDebug()<<localfile;

    if((localfile.endsWith(".ts"))
        || (localfile.endsWith(".mp4", Qt::CaseInsensitive))
        || (localfile.endsWith(".mov", Qt::CaseInsensitive))
        || (localfile.endsWith(".avi", Qt::CaseInsensitive))
        || (localfile.endsWith(".rmvb", Qt::CaseInsensitive))
        || (localfile.endsWith(".rm", Qt::CaseInsensitive))
        || (localfile.endsWith(".flv", Qt::CaseInsensitive))
        || (localfile.endsWith(".3gp", Qt::CaseInsensitive))){
#if defined(Q_OS_WIN)
        QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay.exe";
#else
        QString playerpath = QCoreApplication::applicationDirPath()+"/ffplay";
#endif
        //QString playerpath = QCoreApplication::applicationDirPath()+"/vlc/vlc.exe";
        QStringList args;
        args << "-i" << localfile;
        //args << QString(QStringLiteral("udp://192.168.1.168:5440")); /test
        if(m_playerProcess->isOpen()){
            m_playerProcess->close();
            m_playerProcess->waitForFinished();
        }
        m_playerProcess->start(playerpath,args);
        if(!m_playerProcess->waitForStarted()){
            QMessageBox::warning(this, "warning", QString("player run failed!"));
        }
    }else{
        currentTab()->setUrl(url);

    }
}

void BrowserWindow::handleFindActionTriggered()
{
    if (!currentTab())
        return;
    bool ok = false;
    QString search = QInputDialog::getText(this, tr("Find"),
                                           tr("Find:"), QLineEdit::Normal,
                                           m_lastSearch, &ok);
    if (ok && !search.isEmpty()) {
        m_lastSearch = search;
        currentTab()->findText(m_lastSearch, 0, [this](bool found) {
            if (!found)
                statusBar()->showMessage(tr("\"%1\" not found.").arg(m_lastSearch));
        });
    }
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    if (m_tabWidget->count() > 1) {
        int ret = QMessageBox::warning(this, tr("Confirm close"),
                                       tr("Are you sure you want to close the window ?\n"
                                          "There are %1 tabs open.").arg(m_tabWidget->count()),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::No) {
            event->ignore();
            return;
        }
    }
    this->m_playerProcess->close();
    this->m_playerProcess->waitForFinished();
    this->m_playerProcess->deleteLater();
    this->m_playerProcess = nullptr;

    this->m_recordProcess->close();
    this->m_recordProcess->waitForFinished();
    this->m_recordProcess->deleteLater();
    this->m_recordProcess = nullptr;

    event->accept();
    deleteLater();
}

TabWidget *BrowserWindow::tabWidget() const
{
    return m_tabWidget;
}

WebView *BrowserWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

void BrowserWindow::handleWebViewLoadProgress(int progress)
{
    static QIcon stopIcon(QStringLiteral(":process-stop.png"));
    static QIcon reloadIcon(QStringLiteral(":view-refresh.png"));

    if (0 < progress && progress < 100) {
        m_stopReloadAction->setData(QWebEnginePage::Stop);
        m_stopReloadAction->setIcon(stopIcon);
        m_stopReloadAction->setToolTip(tr("Stop loading the current page"));
        m_progressBar->setValue(progress);
    } else {
        m_stopReloadAction->setData(QWebEnginePage::Reload);
        m_stopReloadAction->setIcon(reloadIcon);
        m_stopReloadAction->setToolTip(tr("Reload the current page"));
        m_progressBar->setValue(0);
    }
}

void BrowserWindow::handleShowWindowTriggered()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int offset = action->data().toInt();
        QVector<BrowserWindow*> windows = m_browser->windows();
        windows.at(offset)->activateWindow();
        windows.at(offset)->currentTab()->setFocus();
    }
}

void BrowserWindow::handleDevToolsRequested(QWebEnginePage *source)
{
    source->setDevToolsPage(m_browser->createDevToolsWindow()->currentTab()->page());
    source->triggerAction(QWebEnginePage::InspectElement);
}

void BrowserWindow::readContent()
{
    // qDebug()<<m_reply->readAll();
    m_file->write(m_reply->readAll());
}

void BrowserWindow::replyFinished(QNetworkReply*)
{
    if(m_reply->error() == QNetworkReply::NoError)
    {
        m_reply->deleteLater();
        m_file->flush();
        m_file->close();
        m_file->deleteLater();
        m_reply = nullptr;
        m_file = nullptr;
    } else{
        m_reply->deleteLater();
        m_file->flush();
        m_file->close();
        m_file->deleteLater();
        m_quit = 1;
        m_reply = nullptr;
        m_file = nullptr;

    }
}

void BrowserWindow::loadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    qDebug() << "loaded" << bytesSent << "of" << bytesTotal;
}

void BrowserWindow::loadError(QNetworkReply::NetworkError)
{
    qDebug()<<"Error: "<<m_reply->error();

}



/*
 ffplay While playing:
q, ESC              quit
f                   toggle full screen
p, SPC              pause
m                   toggle mute
9, 0                decrease and increase volume respectively
/, *                decrease and increase volume respectively
a                   cycle audio channel in the current program
v                   cycle video channel
t                   cycle subtitle channel in the current program
c                   cycle program
w                   cycle video filters or show modes
s                   activate frame-step mode
left/right          seek backward/forward 10 seconds
down/up             seek backward/forward 1 minute
page down/page up   seek backward/forward 10 minutes
right mouse click   seek to percentage in file corresponding to fraction of width
left double-click   toggle full screen
 */


// curl.exe  -F "file=@audioencode.conf" -i http://root:root@192.168.1.234/cgi-bin/conf_upload.cgi

