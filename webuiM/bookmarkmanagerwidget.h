#ifndef BOOKMARKMANAGERWIDGET_H
#define BOOKMARKMANAGERWIDGET_H

#include <QDialog>
#include <QSettings>
#include <QListWidgetItem>
#include <QUrl>
#include <QKeyEvent>
#include <QAction>
#define  DEVNUM  8
namespace Ui {
class BookmarkManagerWidget;
}

class BookmarkManagerWidget : public QDialog
{
    Q_OBJECT
public:
    explicit BookmarkManagerWidget(QWidget *parent = nullptr);
    ~BookmarkManagerWidget();
    int readIni(QString name);
    int writeIni(QString mainpage,QStringList iplst,QString name=QString("Config.ini"));
    QStringList iniIpStringList(QString name=QString("Config.ini"));
    QString iniMainPageString(QString name=QString("Config.ini"));
    QUrl read_MainPageUrl_from_ini(QString name);
    QStringList urlQstrl(void);
    void setWindowOpacityAll(double d);
    void doQObject(QAction *a);
    void updateUI(QString name=QString("Config.ini"));
    void updateDefaultUI(QString name=QString("Config.ini"));
Q_SIGNALS:
    void sig_listWidget_clicked(QUrl url);

protected:
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void keyPressEvent(QKeyEvent  *event);
    void closeEvent(QCloseEvent*event);
    void showEvent(QShowEvent *);
private:
    Ui::BookmarkManagerWidget *ui;
    QStringList urlqstrl;
    QAction *a;
};

#endif // BOOKMARKMANAGERWIDGET_H
