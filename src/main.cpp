/*
 * Copyright (C) 2021  Alfred Neumayer
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * pvms is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QQmlEngine>
#include <QUrl>
#include <QString>
#include <QQuickView>
#include <QQmlContext>

#ifdef PVMS_LEGACY
#include <QIcon>
#endif

int main(int argc, char *argv[])
{
    QApplication *app = new QApplication(argc, (char**)argv);
    app->setApplicationName("pvms.me.fredl");

    qDebug() << "Starting app from main.cpp";

    QQuickView *view = new QQuickView();
    
    // Make our Pocket VMs QML plugin find the QMLTermWidget one
#if __aarch64__
#define TRIPLET "aarch64-linux-gnu"
#else
#define TRIPLET "x86_64-linux-gnu"
#endif
    view->engine()->addImportPath(app->applicationDirPath() + "/usr/lib/" TRIPLET "/qt5/qml");

#ifdef PVMS_LEGACY
    const QString snapPath = qgetenv("SNAP");
    const QString snapThemePath = QStringLiteral("%1/usr/share/icons").arg(snapPath);
    qDebug() << snapThemePath;
    QIcon::setThemeSearchPaths(QStringList() << snapThemePath);
    QIcon::setThemeName("suru");
    view->rootContext()->setContextProperty("legacy", true);
#else
    view->rootContext()->setContextProperty("legacy", false);
#endif
    view->setSource(QUrl("qrc:/Main.qml"));
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->show();

    return app->exec();
}
