﻿#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QProcess>
#include <QStyle>
#include <QDesktopWidget>
#include <QTime>
#include <QPropertyAnimation>
#include <QIcon>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QTimer>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // this->setWindowFlags(Qt::FramelessWindowHint);
    this->setGeometry(
                QStyle::alignedRect(
                    Qt::LeftToRight,
                    Qt::AlignCenter,
                    this->size(),
                    qApp->desktop()->availableGeometry()
                    )
                );
    ui->iconLabel->setPixmap(QIcon::fromTheme("exit").pixmap(48, QIcon::Disabled));

    this->setFixedSize(this->size());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_logoutButton_clicked()
{
    QTimer::singleShot(2500, []() { QProcess::execute("killall", QStringList() << "sh"); } );
    this->hide();
    QTimer::singleShot(250, [this]() { this->fadeToBlack(); } );; // Give the dialog box some time to fade out before we fade out full screen
}

void MainWindow::on_restartButton_clicked()
{
    QTimer::singleShot(2500, []() { QProcess::execute("sudo", QStringList() << "shutdown" << "-r" << "now"); } );
    this->close();
    QTimer::singleShot(250, [this]() { this->fadeToGray(); } );; // Give the dialog box some time to fade out before we fade out full screen
}

void MainWindow::on_shutdownButton_clicked()
{
    QTimer::singleShot(2500, []() { QProcess::execute("sudo", QStringList() << "shutdown" << "-p" << "now"); } );
    this->close();
    QTimer::singleShot(250, [this]() { this->fadeToGray(); } );; // Give the dialog box some time to fade out before we fade out full screen
}

void MainWindow::fadeToGray()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/shutdown.qml"));
    component.create();
}

void MainWindow::fadeToBlack()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/logout.qml"));
    component.create();
}
