/*
 * Copyright (C) 2025  JCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * waydroid-files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QGuiApplication>
#include <QCoreApplication>
#include <QUrl>
#include <QString>
#include <QQuickView>

#include "patharrowbackground.h"
#include "snaphelper.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = new QGuiApplication(argc, (char**)argv);
    app->setApplicationName("waydroid-files.jcm");

    qDebug() << "Starting app from main.cpp";

    qmlRegisterType<PathArrowBackground>("waydroidfiles.jcm", 1, 0, "PathArrowBackground");
    qmlRegisterSingletonType<SnapHelper>("waydroidfiles.jcm", 1, 0, "SnapHelper", [](QQmlEngine *engine, QJSEngine *scriptEngine) -> QObject* {return new SnapHelper;});

    QQuickView *view = new QQuickView();
    view->setSource(QUrl("qrc:/Main.qml"));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->show();

    return app->exec();
}
