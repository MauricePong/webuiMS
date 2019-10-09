#ifndef BROWSER_H
#define BROWSER_H

#include "downloadmanagerwidget.h"
#include "bookmarkmanagerwidget.h"
#include <QVector>
#include <QWebEngineProfile>
class BrowserWindow;
class BookmarkManagerWidget;
class Browser

{
public:
    Browser();

    QVector<BrowserWindow*> windows() { return m_windows; }

    BrowserWindow *createWindow(bool offTheRecord = false);
    BrowserWindow *createDevToolsWindow();

    DownloadManagerWidget &downloadManagerWidget() { return  m_downloadManagerWidget; }
    BookmarkManagerWidget &bookmarkManagerWidget() { return  m_bookmarkManagerWidget;}
private:
    QVector<BrowserWindow*> m_windows;
    DownloadManagerWidget m_downloadManagerWidget;
    BookmarkManagerWidget m_bookmarkManagerWidget;

    QScopedPointer<QWebEngineProfile> m_otrProfile;
};
#endif // BROWSER_H
