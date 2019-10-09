#include "browser.h"
#include "browserwindow.h"
#include "tabwidget.h"
#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QFile>
#include <QSettings>
#include <QNetworkProxyFactory>

QUrl read_MainPageUrl_from_ini()
{
    int file_isexists = 0;
    QUrl mainPage;
    QString Config = QCoreApplication::applicationDirPath() + "/Config.ini";
    QFile file(Config);
    QSettings *ConfigSetting = new QSettings(Config,QSettings::IniFormat);
    ConfigSetting->beginGroup("MainPage");
    if(!file.exists()){
        ConfigSetting->setValue("mainpage","http://root:root@192.168.1.2/");
        mainPage =  QUrl(QStringLiteral("http://root:root@192.168.1.2/"));
        file_isexists = 0;
    }else{
        mainPage = ConfigSetting->value("mainpage").toUrl();
        file_isexists = 1;
        //if(mainPage.isEmpty()){
        //    ConfigSetting->setValue("mainpage","http://root:root@192.168.1.2/");
        //    mainPage =  QUrl(QStringLiteral("http://root:root@192.168.1.2/"));
        //}
    }
    ConfigSetting->endGroup();
    if(0 == file_isexists){
        ConfigSetting->beginGroup("BookmarkManagerWidget");
        for(int i = 0; i < DEVNUM; i++){
            ConfigSetting->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),
                                    QString("http://root:root@192.168.1.%1/").arg(i+2));
        }
        ConfigSetting->endGroup();
    }

    if(nullptr != ConfigSetting){
        delete ConfigSetting;
    }
    return mainPage;
}

QUrl commandLineUrlArgument()
{
    const QStringList args = QCoreApplication::arguments();
    for (const QString &arg : args.mid(1)) {
        if (!arg.startsWith(QLatin1Char('-')))
            return QUrl::fromUserInput(arg);
    }
    return read_MainPageUrl_from_ini();
}

void initStyle()
{
    qApp->setWindowIcon(QIcon(QStringLiteral(":/AppLogoColor.ico")));
    //qApp->setFont(QFont("Microsoft Yahei", 9));
}
int main(int argc, char **argv)
{
    //QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);//no gpu or old gpu
    QCoreApplication::setOrganizationName("TBS");
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QNetworkProxyFactory::setUseSystemConfiguration(false);

    QApplication app(argc, argv);
    initStyle();
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);
    // QWebEngineSettings::globalSettings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    QWebEngineProfile::defaultProfile()->setUseForGlobalCertificateVerification();
    QUrl url = commandLineUrlArgument();

    Browser browser;
    BrowserWindow *window = browser.createWindow();
    window->tabWidget()->setUrl(url);

    return app.exec();
}
