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
            // Checking #1
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

            // Checking #2
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
    if (text.isEmpty() || !manager) return;

    // Checking #1
    ui->textEdit_res->setText("Translating...");

    QString cleanedText = text;
    cleanedText = cleanedText.replace("\n", " ").replace("\r", " ").simplified();

    // Languages
    QMap<QString, QString> langMap = {
      {"Arabic", "ar"},
      {"Bulgarian", "bg"},
      {"Catalan", "ca"},
      {"Chinese (Simplified)", "zh-CN"},
      {"Chinese (Traditional)", "zh-TW"},
      {"Czech", "cs"},
      {"Danish", "da"},
      {"English", "en"},
      {"Finnish", "fi"},
      {"French", "fr"},
      {"Georgian", "ka"},
      {"German", "de"},
      {"Hebrew", "he"},
      {"Hungarian", "hu"},
      {"Italian", "it"},
      {"Japanese", "ja"},
      {"Korean", "ko"},
      {"Latvian", "lv"},
      {"Dutch", "nl"},
      {"Norwegian (Nynorsk)", "nn"},
      {"Persian", "fa"},
      {"Polish", "pl"},
      {"Portuguese (Brazil)", "pt-BR"},
      {"Russian", "ru"},
      {"Slovak", "sk"},
      {"Spanish", "es"},
      {"Swedish", "sv"},
      {"Turkish", "tr"},
      {"Ukrainian", "uk"}
    };
    QString from = langMap.value(ui->comboBox_from->currentText(), "en");
    QString to = langMap.value(ui->comboBox_to->currentText(), "ru");

    // API
    QUrl url("https://translate.googleapis.com/translate_a/single?client=gtx&sl=" + from + "&tl=" + to + "&dt=t&q=" + QUrl::toPercentEncoding(cleanedText));

    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    // Checking #2
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (!reply) return;
        reply->deleteLater();

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);

            if (doc.isArray() && !doc.array().isEmpty()) {
                QString fullResult = "";
                QJsonArray sentences = doc.array().at(0).toArray();

                for (int i = 0; i < sentences.size(); ++i) {
                    fullResult += sentences.at(i).toArray().at(0).toString();
                }

                if (!fullResult.isEmpty()) {
                    ui->textEdit_res->setText(fullResult);
                    QGuiApplication::clipboard()->blockSignals(true);
                    QGuiApplication::clipboard()->setText(fullResult);
                    QGuiApplication::clipboard()->blockSignals(false);
                }
            }
        } else {
            ui->textEdit_res->setText("Error: " + reply->errorString());
        }
    });
}
