#include "bookmarkmanagerwidget.h"
#include "ui_bookmarkmanagerwidget.h"
#include <QFile>
#include <QDebug>

BookmarkManagerWidget::BookmarkManagerWidget(QWidget *parent) :
      QDialog(parent),
      ui(new Ui::BookmarkManagerWidget)
{
    ui->setupUi(this);
    this->setWindowTitle("Bookmark");
    this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
    //this->setWindowFlags(this->windowFlags() | Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    //this->setWindowFlags(this->windowFlags()| Qt::FramelessWindowHint);
    // ui->listWidget->setViewMode(QListView::IconMode);
    this->setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);

    ui->listWidget->setViewMode(QListView::ListMode);
    // QObject::connect(ui->listWidget,SIGNAL(itemDoubleClicked(QListWidgetItem*)),this,SLOT(doubleclicked(QListWidgetItem*)));
    this->setWindowOpacity(0.9);
    ui->listWidget->setWindowOpacity(0.9);
    readIni(QString("Config.ini"));
    this->a = nullptr;
    connect(ui->listWidget, &QListWidget::itemClicked, [this]() {
        emit sig_listWidget_clicked(QUrl(this->urlqstrl.at(ui->listWidget->currentRow())));
    });
}

BookmarkManagerWidget::~BookmarkManagerWidget()
{
    delete ui;
}

int BookmarkManagerWidget::readIni(QString name)
{
    QString path = QCoreApplication::applicationDirPath()+"/" + name;
    QFile f(path);
    QStringList strl;
    urlqstrl.clear();
    QSettings *Settings = new QSettings(path,QSettings::IniFormat);
    Settings->beginGroup("BookmarkManagerWidget");
    if(!f.exists()){
        for(int i = 0; i < DEVNUM; i++){
            strl << QString("device %1").arg(i,2,10,QLatin1Char('0'));
            urlqstrl << QString("http://root:root@192.168.1.%1/").arg(i+2);
            Settings->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),QString("http://root:root@192.168.1.%1/").arg(i+2));
        }
    }else{
        QStringList keyl = Settings->allKeys();
        //if(keyl.isEmpty()){
        //    for(int i = 0; i < DEVNUM; i++){
        //        strl << QString("device %1").arg(i,2,10,QLatin1Char('0'));
        //        urlqstrl << QString("http://root:root@192.168.1.%1/").arg(i+2);
        //        Settings->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),QString("http://root:root@192.168.1.%1/").arg(i+2));
        //    }
       // }else{
            strl = keyl;
            foreach(QString key,keyl){
                urlqstrl << Settings->value(key).toString();
            }
       // }
    }
    Settings->endGroup();
    ui->listWidget->clear();
    ui->listWidget->addItems(strl);
    if(nullptr != Settings){
        delete Settings;
    }
    return 0;
}

int BookmarkManagerWidget::writeIni(QString mainpage, QStringList iplst, QString name)
{
    QString path = QCoreApplication::applicationDirPath()+"/" + name;
    QFile::remove(path);
    QStringList strl;
    urlqstrl.clear();
    QSettings *Settings = new QSettings(path,QSettings::IniFormat);
    Settings->beginGroup("BookmarkManagerWidget");
    for(int i = 0;i < iplst.size();i++){
        Settings->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),iplst.at(i));
    }
    Settings->endGroup();

    Settings->beginGroup("MainPage");
    Settings->setValue("mainpage",mainpage);
    Settings->endGroup();
    ui->listWidget->addItems(strl);
    if(nullptr != Settings){
        delete Settings;
    }
    return 0;
}

QStringList BookmarkManagerWidget::iniIpStringList(QString name)
{
    QString path = QCoreApplication::applicationDirPath()+"/" + name;
    QFile f(path);
    urlqstrl.clear();
    QSettings *Settings = new QSettings(path,QSettings::IniFormat);
    Settings->beginGroup("BookmarkManagerWidget");
    if(!f.exists()){
        for(int i = 0; i < DEVNUM; i++){
            urlqstrl << QString("http://root:root@192.168.1.%1/").arg(i+2);
            Settings->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),QString("http://root:root@192.168.1.%1/").arg(i+2));
        }
    }else{
        QStringList keyl = Settings->allKeys();
        //if(keyl.isEmpty()){
        //    for(int i = 0; i < DEVNUM; i++){
        //        urlqstrl << QString("http://root:root@192.168.1.%1/").arg(i+2);
        //        Settings->setValue(QString("device %1").arg(i,2,10,QLatin1Char('0')),QString("http://root:root@192.168.1.%1/").arg(i+2));
        //    }
        //}else{
            foreach(QString key,keyl){
                urlqstrl << Settings->value(key).toString();
            }
       // }
    }
    Settings->endGroup();
    if(nullptr != Settings){
        delete Settings;
    }
    return urlqstrl;
}

QString BookmarkManagerWidget::iniMainPageString(QString name)
{
    QString path = QCoreApplication::applicationDirPath()+"/" + name;
    QString str;
    QSettings *Settings = new QSettings(path,QSettings::IniFormat);
    Settings->beginGroup("MainPage");
    QStringList keyl = Settings->allKeys();
    str = Settings->value(keyl.at(0)).toString();
    if(str.isEmpty()){
        str = urlqstrl.at(0);
    }
    Settings->endGroup();
    if(nullptr != Settings){
        delete Settings;
    }
    return str;
}

QUrl BookmarkManagerWidget::read_MainPageUrl_from_ini(QString name)
{
    QUrl mainPage;
    QString Config = QCoreApplication::applicationDirPath() + "/"+name;
    QFile file(Config);
    QSettings *ConfigSetting = new QSettings(Config,QSettings::IniFormat);
    ConfigSetting->beginGroup("MainPage");
    if(!file.exists()){
        ConfigSetting->setValue("mainpage","http://root:root@192.168.1.2/");
        mainPage =  QUrl(QStringLiteral("http://root:root@192.168.1.2/"));
    }else{
        mainPage = ConfigSetting->value("mainpage").toUrl();
        //if(mainPage.isEmpty()){
        //    ConfigSetting->setValue("mainpage","http://root:root@192.168.1.2/");
        //    mainPage =  QUrl(QStringLiteral("http://root:root@192.168.1.2/"));
        //}
    }
    ConfigSetting->endGroup();
    if(nullptr != ConfigSetting){
        delete ConfigSetting;
    }
    return mainPage;
}

void BookmarkManagerWidget::setWindowOpacityAll(double d)
{
    this->setWindowOpacity(d);
    ui->listWidget->setWindowOpacity(d);
}

void BookmarkManagerWidget::doQObject(QAction *a)
{
    //a->setText(tr("Show Bookmark"));
    this->a = a;
}

void BookmarkManagerWidget::updateUI(QString name)
{
    readIni(name);
    read_MainPageUrl_from_ini(name);
}

void BookmarkManagerWidget::updateDefaultUI(QString name)
{
    QString path = QCoreApplication::applicationDirPath()+"/"+name;
    QFile f(path);
    f.remove();
    updateUI(name);
}

void BookmarkManagerWidget::enterEvent(QEvent *)
{
    this->setWindowOpacity(0.9);
    ui->listWidget->setWindowOpacity(0.9);
}

void BookmarkManagerWidget::leaveEvent(QEvent *)
{
    // int offsetx = QCursor::pos().x() - this->pos().x();
    // int offsety = this->pos().y() - QCursor::pos().y();

    // if(offsetx > 60 || offsety > 20){
    this->setWindowOpacity(0.1);
    ui->listWidget->setWindowOpacity(0.1);
    // }
}

void BookmarkManagerWidget::keyPressEvent(QKeyEvent *event)
{
    if ((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_B)){
        if(this->isVisible()){
            if(nullptr != this->a)
                this->a->setText(tr("Show Bookmark"));
            this->close();

        }else{
            if(nullptr != this->a)
                this->a->setText(tr("Hide Bookmark"));
            this->show();
        }
    }

}

void BookmarkManagerWidget::closeEvent(QCloseEvent *event)
{
    if(nullptr != this->a)
        this->a->setText(tr("Show Bookmark"));
}

void BookmarkManagerWidget::showEvent(QShowEvent *)
{
    if(nullptr != this->a)
        this->a->setText(tr("Hide Bookmark"));
}

