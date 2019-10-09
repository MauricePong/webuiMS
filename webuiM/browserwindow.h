#ifndef BROWSERWINDOW_H
#define BROWSERWINDOW_H

#include <QMainWindow>
#include <QTime>
#include <QWebEnginePage>
#include <QProcess>
#include <QNetworkReply>
#include <QFile>
#include <QUdpSocket>
QT_BEGIN_NAMESPACE
class QLineEdit;
class QProgressBar;
QT_END_NAMESPACE

class Browser;
class TabWidget;
class WebView;

#if defined(Q_OS_WIN)
#include <Windows.h>
#define QMSLEEP(n)    Sleep(n)
#else
#include <unistd.h>
#define QMSLEEP(n)    usleep(1000*n)
#endif

class BrowserWindow : public QMainWindow
{
    Q_OBJECT

public:
    BrowserWindow(Browser *browser, QWebEngineProfile *profile, bool forDevTools = false);
    QSize sizeHint() const override;
    TabWidget *tabWidget() const;
    WebView *currentTab() const;
    Browser *browser() { return m_browser; }

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void handleNewWindowTriggered();
    void handleNewIncognitoWindowTriggered();
    void handleFileOpenTriggered();
    void handleFindActionTriggered();
    void handleShowWindowTriggered();
    void handleWebViewLoadProgress(int);
    void handleWebViewTitleChanged(const QString &title);
    void handleWebActionEnabledChanged(QWebEnginePage::WebAction action, bool enabled);
    void handleDevToolsRequested(QWebEnginePage *source);

    void  readContent(void);
    void  replyFinished(QNetworkReply *);
    void  loadProgress(qint64 bytesSent,qint64 bytesTotal);
    void  loadError(QNetworkReply::NetworkError);
private:
    QMenu *createFileMenu(TabWidget *tabWidget);
    QMenu *createEditMenu();
    QMenu *createViewMenu(QToolBar *toolBar);
    QMenu *createWindowMenu(TabWidget *tabWidget);
    QMenu *createHelpMenu();
    QToolBar *createToolBar();

    bool makeDirPath(QString dirPath);
    bool deleteDirectory(const QString &path);
private:
    Browser *m_browser;
    QWebEngineProfile *m_profile;
    TabWidget *m_tabWidget;
    QProgressBar *m_progressBar;
    QAction *m_historyBackAction;
    QAction *m_historyForwardAction;
    QAction *m_stopAction;
    QAction *m_reloadAction;
    QAction *m_stopReloadAction;
    QLineEdit *m_urlLineEdit;
    QAction *m_favAction;
    QString m_lastSearch;
    QProcess *m_playerProcess;
    QProcess *m_recordProcess;
    QNetworkReply *m_reply;

    QString m_linedit_text_bak;
    QFile *m_file;
    QUdpSocket* m_udprecv;
    QStringList m_udprecvsl;
    int m_quit;
};

#endif // BROWSERWINDOW_H
