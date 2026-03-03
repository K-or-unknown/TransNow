#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QThread>
#include <QKeyEvent>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QIcon>
#include <QAction>
#include <QMenu>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Window settings
    this->setWindowTitle("TransNow");
    this->setWindowIcon(QIcon(":/icon.ico"));
    ui->lineEdit_bind_trans->installEventFilter(this);
    ui->lineEdit_bind_trans->setReadOnly(true);
    // Creating Network Manager
    manager = new QNetworkAccessManager(this);
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    // Creating icon for system tray
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    trayIcon->setToolTip("TransNow");

    // Creating menu
    QMenu *menu = new QMenu(this);
    QAction *quitAction = new QAction("Exit", this);

    menu->addAction(quitAction);
    trayIcon->setContextMenu(menu);

    // Completely close the program by clicking "Exit" in the menu.
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);

    // Show/Hide the window by clicking on the icon itself
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) { // Single left-click
            if (this->isVisible()) {
                this->hide();
            } else {
                this->showNormal();
                this->activateWindow();
            }
        }
    });
    trayIcon->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Hide window (button)
void MainWindow::on_pushButton_clicked()
{
    this->hide();
}

// Bind function for translating
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->lineEdit_bind_trans && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        currentVkCode = keyEvent->nativeVirtualKey();

        QString keyName = QKeySequence(keyEvent->key()).toString();
        ui->lineEdit_bind_trans->setText(keyName);

        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}

// Button accepting a bind
void MainWindow::on_pushButton_apply_clicked()
{

        if (currentVkCode == 0) return;

        UnregisterHotKey((HWND)this->winId(), 1);

        RegisterHotKey((HWND)this->winId(), 1, 0, currentVkCode);

}

// Function of copying
bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_HOTKEY) {
        if (msg->wParam == 1) {
            // 1. Checking
            QGuiApplication::clipboard()->setText("---WAIT---");
            QCoreApplication::processEvents();

            INPUT inputs[4] = {};

            // Press Ctrl
            inputs[0].type = INPUT_KEYBOARD;
            inputs[0].ki.wVk = VK_CONTROL;

            // Press C
            inputs[1].type = INPUT_KEYBOARD;
            inputs[1].ki.wVk = 'C';

            // Release C
            inputs[2].type = INPUT_KEYBOARD;
            inputs[2].ki.wVk = 'C';
            inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

            // Release Ctrl
            inputs[3].type = INPUT_KEYBOARD;
            inputs[3].ki.wVk = VK_CONTROL;
            inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

            SendInput(4, inputs, sizeof(INPUT));

            bool success = false;

            // 2. Checking
            for (int i = 0; i < 10; ++i) {
                QThread::msleep(50);
                QCoreApplication::processEvents();
                QString currentText = QGuiApplication::clipboard()->text();
                if (!currentText.isEmpty() && currentText != "---WAIT---") {
                    success = true;
                    translateFunction(currentText);
                    break;
                }
            }

            if (!success) {
                qDebug() << "Copying text didn't work";
            }
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}

// Translate function
void MainWindow::translateFunction(const QString &text) {
    // 1. Checking
    if (text.isEmpty() || !manager) return;

    ui->line_edit_res->setText("Request...");

    QString from = (ui->comboBox_from->currentText() == "Russian") ? "ru" : "en";
    QString to = (ui->comboBox_to->currentText() == "Russian") ? "ru" : "en";

    // API
    QUrl url("https://translate.googleapis.com/translate_a/single?client=gtx&sl=" + from + "&tl=" + to + "&dt=t&q=" + QUrl::toPercentEncoding(text));

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    // 2. Checking
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (!reply) return;

        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isArray() && !doc.array().isEmpty()) {
                QString res = doc.array().at(0).toArray().at(0).toArray().at(0).toString();

                if (!res.isEmpty()) {
                    ui->line_edit_res->setText(res);

                    QGuiApplication::clipboard()->blockSignals(true);
                    QGuiApplication::clipboard()->setText(res);
                    QGuiApplication::clipboard()->blockSignals(false);
                }
            }
        } else {
            ui->line_edit_res->setText("Error: " + reply->errorString());
        }
    });
}
