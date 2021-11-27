/*
 * Copyright (C) 2020 Alberto Mardegan <mardy@users.sourceforge.net>
 *
 * This file is part of LomiriVNC.
 *
 * LomiriVNC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LomiriVNC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LomiriVNC.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "vnc_client.h"

#include <QByteArrayList>
#include <QDebug>
#include <QImage>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPointF>
#include <QQuickItem>
#include <QScopedPointer>
#include <QSocketNotifier>
#define XK_CYRILLIC
#include <rfb/rfbclient.h>

using namespace LomiriVNC;

namespace LomiriVNC {

class VncClientPrivate {
    Q_DECLARE_PUBLIC(VncClient)

public:
    VncClientPrivate(VncClient *q);
    ~VncClientPrivate();

    static void *dataTag();
    static void vncLog(const char *format, ...);
    static void vncError(const char *format, ...);
    static void gotFrameBufferUpdate(rfbClient *cl,
                                     int x, int y, int w, int h);
    static char *GetPassword(rfbClient *cl);
    static rfbBool mallocFrameBuffer(rfbClient* client);

    static int qtToRfb(Qt::MouseButtons buttons);
    static uint32_t qCharToVnc(QChar c);
    static uint32_t qKeyToVnc(int key);

    void onUpdate(int x, int y, int w, int h);
    void onResize();
    char *getPassword();

    bool connectToServer(const QString &host, const QString &password);
    void disconnect();

    void onSocketActivated();

    void sendKeyEvent(QChar c);
    void sendKeyEvent(QKeyEvent *keyEvent, bool pressed);
    void sendKeyEvent(uint32_t code, bool pressed);
    void sendMouseEvent(const QPointF &pos, Qt::MouseButtons buttons);

private:
    QScopedPointer<QSocketNotifier> m_notifier;
    int m_bytesPerPixel;
    QImage m_image;
    QList<QQuickItem*> m_viewers;
    QString m_password;
    rfbClient *m_client;
    VncClient *q_ptr;
};

} // namespace

VncClientPrivate::VncClientPrivate(VncClient *q):
    m_bytesPerPixel(4),
    m_client(nullptr),
    q_ptr(q)
{
    rfbClientLog = vncLog;
    rfbClientErr = vncError;
}

VncClientPrivate::~VncClientPrivate()
{
    disconnect();
}

void *VncClientPrivate::dataTag()
{
    return reinterpret_cast<void*>(dataTag);
}

void VncClientPrivate::vncLog(const char *format, ...)
{
    va_list args;
	va_start(args, format);
    QString message = QString::vasprintf(format, args);
	va_end(args);

    qDebug() << "VNC:" << message.trimmed();
}

void VncClientPrivate::vncError(const char *format, ...)
{
    va_list args;
	va_start(args, format);
    QString message = QString::vasprintf(format, args);
	va_end(args);

    qWarning() << "VNC:" << message.trimmed();
}

void VncClientPrivate::gotFrameBufferUpdate(rfbClient *client,
                                            int x, int y, int w, int h)
{
    void *ptr = rfbClientGetClientData(client, dataTag());
    static_cast<VncClientPrivate*>(ptr)->onUpdate(x, y, w, h);
}

char *VncClientPrivate::GetPassword(rfbClient *client)
{
    void *ptr = rfbClientGetClientData(client, dataTag());
    return static_cast<VncClientPrivate*>(ptr)->getPassword();
}

rfbBool VncClientPrivate::mallocFrameBuffer(rfbClient* client)
{
    void *ptr = rfbClientGetClientData(client, dataTag());
    static_cast<VncClientPrivate*>(ptr)->onResize();
    return true;
}

int VncClientPrivate::qtToRfb(Qt::MouseButtons buttons)
{
    int ret = 0;
    if (buttons & Qt::LeftButton) ret |= rfbButton1Mask;
    if (buttons & Qt::RightButton) ret |= rfbButton2Mask;
    if (buttons & Qt::MiddleButton) ret |= rfbButton3Mask;
    return ret;
}

#include "key_mapping.h"

uint32_t VncClientPrivate::qCharToVnc(QChar c)
{
    uint32_t code = 0;

    if (char latin1 = c.toLatin1()) {
        code = XK_a + latin1 - 'a';
    }

    if (code == 0) {
        uint16_t unicode = c.unicode();
        for (UnicodeKeyEntry *i = unicodeToX11; i->unicode != 0; i++) {
            if (i->unicode == unicode) {
                code = i->xKey;
                break;
            }
        }
        qDebug() << "char" << c << "unicode" << code;
    }
    return code;
}

uint32_t VncClientPrivate::qKeyToVnc(int key)
{
    uint32_t code = 0;
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        code = XK_0 + key - Qt::Key_0;
    } else if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        code = XK_a + key - Qt::Key_A;
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F35) {
        code = XK_F1 + key - Qt::Key_F1;
    } else {
        switch (key) {
        case Qt::Key_Escape: code = XK_Escape; break;
        case Qt::Key_Tab: code = XK_Tab; break;
        case Qt::Key_Backspace: code = XK_BackSpace; break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            code = XK_Return;
            break;
        case Qt::Key_Insert: code = XK_Insert; break;
        case Qt::Key_Delete: code = XK_Delete; break;
        case Qt::Key_Pause: code = XK_Pause; break;
        case Qt::Key_Print: code = XK_Print; break;
        case Qt::Key_SysReq: code = XK_Sys_Req; break;
        case Qt::Key_Clear: code = XK_Clear; break;
        case Qt::Key_Home: code = XK_Home; break;
        case Qt::Key_End: code = XK_End; break;
        case Qt::Key_Left: code = XK_Left; break;
        case Qt::Key_Up: code = XK_Up; break;
        case Qt::Key_Right: code = XK_Right; break;
        case Qt::Key_Down: code = XK_Down; break;
        case Qt::Key_PageUp: code = XK_Page_Up; break;
        case Qt::Key_PageDown: code = XK_Page_Down; break;
        case Qt::Key_Shift: code = XK_Shift_L; break;
        case Qt::Key_Control: code = XK_Control_L; break;
        case Qt::Key_Meta: code = XK_Meta_L; break;
        case Qt::Key_Alt: code = XK_Alt_L; break;
        case Qt::Key_AltGr: code = XK_Alt_R; break;
        case Qt::Key_CapsLock: code = XK_Caps_Lock; break;
        case Qt::Key_NumLock: code = XK_Num_Lock; break;
        case Qt::Key_ScrollLock: code = XK_Scroll_Lock; break;
        case Qt::Key_Super_L: code = XK_Super_L; break;
        case Qt::Key_Super_R: code = XK_Super_R; break;
        case Qt::Key_Menu: code = XK_Menu; break;
        case Qt::Key_Hyper_L: code = XK_Hyper_L; break;
        case Qt::Key_Hyper_R: code = XK_Hyper_R; break;
        case Qt::Key_Help: code = XK_Help; break;
        case Qt::Key_Exclam: code = XK_exclam; break;
        case Qt::Key_QuoteDbl: code = XK_quotedbl; break;
        case Qt::Key_NumberSign: code = XK_numbersign; break;
        case Qt::Key_Dollar: code = XK_dollar; break;
        case Qt::Key_Percent: code = XK_percent; break;
        case Qt::Key_Ampersand: code = XK_ampersand; break;
        case Qt::Key_Apostrophe: code = XK_apostrophe; break;
        case Qt::Key_ParenLeft: code = XK_parenleft; break;
        case Qt::Key_ParenRight: code = XK_parenright; break;
        case Qt::Key_Asterisk: code = XK_asterisk; break;
        case Qt::Key_Plus: code = XK_plus; break;
        case Qt::Key_Comma: code = XK_comma; break;
        case Qt::Key_Minus: code = XK_minus; break;
        case Qt::Key_Period: code = XK_period; break;
        case Qt::Key_Slash: code = XK_slash; break;

        default:
            qWarning() << "Unsupported key:" << key;
        }
    }
    return code;
}

void VncClientPrivate::onUpdate(int x, int y, int w, int h)
{
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(w);
    Q_UNUSED(h);

    for (QQuickItem *viewer: m_viewers) {
        // TODO: update only the changed area
        viewer->update();
    }
}

void VncClientPrivate::onResize()
{
    int width = m_client->width;
    int height = m_client->height;
    qDebug() << Q_FUNC_INFO << width << height;

    m_client->updateRect.x = m_client->updateRect.y = 0;
    m_client->updateRect.w = width;
    m_client->updateRect.h = height;

    m_image = QImage(m_client->width, m_client->height, QImage::Format_RGB32);
    m_client->frameBuffer = m_image.bits();
    m_client->width = m_image.bytesPerLine() / m_bytesPerPixel;
    m_client->format.bitsPerPixel = m_image.depth();
    m_client->format.redShift=16;
    m_client->format.greenShift=8;
    m_client->format.blueShift=0;
    m_client->format.redMax=0xff;
    m_client->format.greenMax=0xff;
    m_client->format.blueMax=0xff;
    bool ok = SetFormatAndEncodings(m_client);
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "Could not set format to server";
    }
}

char *VncClientPrivate::getPassword()
{
	return strdup(m_password.toUtf8().constData());
}

bool VncClientPrivate::connectToServer(const QString &host, const QString &password)
{
    Q_Q(VncClient);

    if (m_client) {
        disconnect();
    }

    m_password = QString(password);

    m_client = rfbGetClient(8, 3, m_bytesPerPixel);
    m_client->MallocFrameBuffer = mallocFrameBuffer;
    m_client->GotFrameBufferUpdate = gotFrameBufferUpdate;
    m_client->GetPassword = GetPassword;
    rfbClientSetClientData(m_client, dataTag(), this);

    QByteArrayList arguments = {
        "lomiri-vnc",
    };
    arguments.append(host.toUtf8());
    int argc = arguments.count();
    QVector<char *> argv;
    argv.reserve(argc + 1);
    for (const QByteArray &a: arguments) {
        argv.append((char*)a.constData());
    }
    argv.append(nullptr);

    bool ok = rfbInitClient(m_client, &argc, argv.data());
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "Could not initialize rfbClient";
        m_client = nullptr;
        return false;
    }

    m_notifier.reset(new QSocketNotifier(m_client->sock,
                                         QSocketNotifier::Read));
    QObject::connect(m_notifier.data(), &QSocketNotifier::activated,
                     q, [this]() { onSocketActivated(); });
    Q_EMIT q->connectionStatusChanged();
    return true;
}

void VncClientPrivate::disconnect()
{
    Q_Q(VncClient);

    m_notifier.reset();
    if (m_client) {
        rfbClientCleanup(m_client);
        m_client = nullptr;
    }

    Q_EMIT q->connectionStatusChanged();
}

void VncClientPrivate::onSocketActivated()
{
    bool ok = HandleRFBServerMessage(m_client);
    if (Q_UNLIKELY(!ok)) {
        qWarning() << "RFB failed to handle message";
    }
}

void VncClientPrivate::sendKeyEvent(QChar c)
{
    uint32_t code = qCharToVnc(c);
    sendKeyEvent(code, true);
    sendKeyEvent(code, false);
}

void VncClientPrivate::sendKeyEvent(QKeyEvent *keyEvent, bool pressed)
{
    // TODO handle modifiers
    uint32_t code = qKeyToVnc(keyEvent->key());
    sendKeyEvent(code, pressed);
}

void VncClientPrivate::sendKeyEvent(uint32_t code, bool pressed)
{
    if (Q_UNLIKELY(!m_client)) {
        qWarning() << "Not connected";
        return;
    }
    SendKeyEvent(m_client, code, pressed);
}

void VncClientPrivate::sendMouseEvent(const QPointF &pos,
                                      Qt::MouseButtons buttons)
{
    if (Q_UNLIKELY(!m_client)) {
        qWarning() << "Not connected";
        return;
    }
    SendPointerEvent(m_client, pos.x(), pos.y(), qtToRfb(buttons));
}

VncClient::VncClient(QObject *parent):
    QObject(parent),
    d_ptr(new VncClientPrivate(this))
{
}

VncClient::~VncClient() = default;

bool VncClient::isConnected() const
{
    Q_D(const VncClient);
    return d->m_client != nullptr;
}

void VncClient::addViewer(QQuickItem *viewer)
{
    Q_D(VncClient);
    d->m_viewers.append(viewer);
}

void VncClient::removeViewer(QQuickItem *viewer)
{
    Q_D(VncClient);
    d->m_viewers.removeAll(viewer);
}

const QImage &VncClient::image() const
{
    Q_D(const VncClient);
    return d->m_image;
}

bool VncClient::connectToServer(const QString &host, const QString &password)
{
    Q_D(VncClient);
    return d->connectToServer(host, password);
}

void VncClient::disconnect()
{
    Q_D(VncClient);
    return d->disconnect();
}

void VncClient::sendKeyEvent(QChar c)
{
    Q_D(VncClient);
    return d->sendKeyEvent(c);
}

void VncClient::sendKeyEvent(QKeyEvent *keyEvent, bool pressed)
{
    Q_D(VncClient);
    return d->sendKeyEvent(keyEvent, pressed);
}

void VncClient::sendMouseEvent(const QPointF &pos, Qt::MouseButtons buttons)
{
    Q_D(VncClient);
    return d->sendMouseEvent(pos, buttons);
}
