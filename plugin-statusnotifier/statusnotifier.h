#ifndef STATUSNOTIFIER_H
#define STATUSNOTIFIER_H

#include "pluginsiterface.h"
#include "statusnotifierwidget.h"
#include <QObject>

class Statusnotifier : public QObject, MenuBarExtension
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.panda.menubar/1.0")
    Q_INTERFACES(MenuBarExtension)

public:
    explicit Statusnotifier(QObject *parent = nullptr);

    QString pluginName() override { return "statusnotifier"; }
    QString displayName() override { return QString(); }
    QWidget *itemWidget() override { return new StatusNotifierWidget; }
};

#endif
