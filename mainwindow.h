#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#define WIN32_LEAN_AND_MEAN

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QNetworkAccessManager>
#include <windows.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();
    void on_pushButton_apply_clicked();

private:
    Ui::MainWindow *ui;
    QSystemTrayIcon *trayIcon;
    UINT currentVkCode = 0;
    QNetworkAccessManager *manager;
    void translateFunction(const QString &text);
protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
};
#endif // MAINWINDOW_H
