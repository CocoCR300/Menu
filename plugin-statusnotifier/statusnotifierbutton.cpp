/* BEGIN_COMMON_COPYRIGHT_HEADER
 * (c)LGPL2+
 *
 * Authors:
 *  Balázs Béla <balazsbela[at]gmail.com>
 *  Paulo Lieuthier <paulolieuthier@gmail.com>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

#include "statusnotifierbutton.h"

#include <QDir>
#include <QFile>
#include "../src/appmenu/appmenumodel.h"
#include <dbusmenu-qt5/dbusmenuimporter.h>
#include "sniasync.h"
#include <QIcon>



StatusNotifierButton::StatusNotifierButton(QString service, QString objectPath, QWidget *parent)
    : QToolButton(parent),
    m_menu(nullptr),
    m_status(Passive),
    m_fallbackIcon(QIcon::fromTheme(QLatin1String("application-x-executable")))
{
    this->setStyleSheet("margin: 0px; border: 0px; padding: 0px; border-radius: 0px;"); // probono
    this->setFixedSize(16, 16); // probono

    this->setFocusPolicy(Qt::ClickFocus); // probono: Do not get here by using the tab key, prevent focus stealing away from Action Search

    // setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // probono: Does this do anything?
    setAutoRaise(true);
    interface = new SniAsync(service, objectPath, QDBusConnection::sessionBus(), this);

    connect(interface, &SniAsync::NewIcon, this, &StatusNotifierButton::newIcon);
    connect(interface, &SniAsync::NewOverlayIcon, this, &StatusNotifierButton::newOverlayIcon);
    connect(interface, &SniAsync::NewAttentionIcon, this, &StatusNotifierButton::newAttentionIcon);
    connect(interface, &SniAsync::NewToolTip, this, &StatusNotifierButton::newToolTip);
    connect(interface, &SniAsync::NewStatus, this, &StatusNotifierButton::newStatus);

    interface->propertyGetAsync(QLatin1String("Menu"), [this] (QDBusObjectPath path) {
        if (!path.path().startsWith("/NO_DBUSMENU")) {
            DBusMenuImporter * imp = new DBusMenuImporter{interface->service(), path.path(), this};
            m_menu =  imp->menu();
            m_menu->setObjectName(QLatin1String("StatusNotifierMenu"));



    }
    });

    interface->propertyGetAsync(QLatin1String("Status"), [this] (QString status) {
        newStatus(status);
    });

    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        //do the logic of icons after we've got the theme path
        refetchIcon(Active, value);
        refetchIcon(Passive, value);
        refetchIcon(NeedsAttention, value);
    });

    newToolTip();
}

StatusNotifierButton::~StatusNotifierButton()
{
    delete interface;
}

void StatusNotifierButton::newIcon()
{
    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(Passive, value);
    });
}

void StatusNotifierButton::newOverlayIcon()
{
    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(Active, value);
    });
}

void StatusNotifierButton::newAttentionIcon()
{
    interface->propertyGetAsync(QLatin1String("IconThemePath"), [this] (QString value) {
        refetchIcon(NeedsAttention, value);
    });
}

QImage StatusNotifierButton::convertToGrayScale(const QImage &srcImage) {
     // Convert to 32bit pixel format
     QImage dstImage = srcImage.convertToFormat(srcImage.hasAlphaChannel() ?
              QImage::Format_ARGB32 : QImage::Format_RGB32);

     unsigned int *data = (unsigned int*)dstImage.bits();
     int pixelCount = dstImage.width() * dstImage.height();

     // Convert each pixel to grayscale
     for(int i = 0; i < pixelCount; ++i) {
        int val = qGray(*data);
        *data = qRgba(val, val, val, qAlpha(*data));
        ++data;
     }

     return dstImage;
  }

void StatusNotifierButton::refetchIcon(Status status, const QString& themePath)
{
    QString nameProperty, pixmapProperty;
    if (status == Active)
    {
        nameProperty = QLatin1String("OverlayIconName");
        pixmapProperty = QLatin1String("OverlayIconPixmap");
    }
    else if (status == NeedsAttention)
    {
        nameProperty = QLatin1String("AttentionIconName");
        pixmapProperty = QLatin1String("AttentionIconPixmap");
    }
    else // status == Passive
    {
        nameProperty = QLatin1String("IconName");
        pixmapProperty = QLatin1String("IconPixmap");
    }

    interface->propertyGetAsync(nameProperty, [this, status, pixmapProperty, themePath] (QString iconName) {
        QIcon nextIcon;
        if (!iconName.isEmpty())
        {
            if (QIcon::hasThemeIcon(iconName)){
                // nextIcon = QIcon::fromTheme(iconName);
                nextIcon.addPixmap(QPixmap::fromImage(convertToGrayScale(QIcon::fromTheme(iconName).pixmap(16, 16).toImage())));

            }
            else
            {
                QDir themeDir(themePath);
                if (themeDir.exists())
                {
                    if (themeDir.exists(iconName + QStringLiteral(".png")))
                        // nextIcon.addFile(themeDir.filePath(iconName + QStringLiteral(".png")));
                        nextIcon.addPixmap(QPixmap::fromImage(convertToGrayScale(QImage(themeDir.filePath(iconName + QStringLiteral(".png"))))));

                    if (themeDir.cd(QStringLiteral("hicolor")) || (themeDir.cd(QStringLiteral("icons")) && themeDir.cd(QStringLiteral("hicolor"))))
                    {
                        const QStringList sizes = themeDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
                        for (const QString &dir : sizes)
                        {
                            const QStringList dirs = QDir(themeDir.filePath(dir)).entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
                            for (const QString &innerDir : dirs)
                            {
                                QString file = themeDir.absolutePath() + QLatin1Char('/') + dir + QLatin1Char('/') + innerDir + QLatin1Char('/') + iconName + QStringLiteral(".png");
                                if (QFile::exists(file))
                                    // nextIcon.addFile(file);
                                    nextIcon.addPixmap(QPixmap::fromImage(convertToGrayScale(QImage(file))));

                            }
                        }
                    }
                }
            }

            switch (status)
            {
                case Active:
                    m_overlayIcon = nextIcon;
                    break;
                case NeedsAttention:
                    m_attentionIcon = nextIcon;
                    break;
                case Passive:
                    m_icon = nextIcon;
                    break;
            }

            resetIcon();
        }
        else
        {
            interface->propertyGetAsync(pixmapProperty, [this, status, pixmapProperty] (IconPixmapList iconPixmaps) {
                if (iconPixmaps.empty())
                    return;

                QIcon nextIcon;

                for (IconPixmap iconPixmap: iconPixmaps)
                {
                    if (!iconPixmap.bytes.isNull())
                    {
                        QImage image((uchar*) iconPixmap.bytes.data(), iconPixmap.width,
                                     iconPixmap.height, QImage::Format_ARGB32);

                        const uchar *end = image.constBits() + image.sizeInBytes();
                        uchar *dest = reinterpret_cast<uchar*>(iconPixmap.bytes.data());
                        for (const uchar *src = image.constBits(); src < end; src += 4, dest += 4)
                            qToUnaligned(qToBigEndian<quint32>(qFromUnaligned<quint32>(src)), dest);

                        nextIcon.addPixmap(QPixmap::fromImage(convertToGrayScale(image)));
                    }
                }

                switch (status)
                {
                    case Active:
                        m_overlayIcon = nextIcon;
                        break;
                    case NeedsAttention:
                        m_attentionIcon = nextIcon;
                        break;
                    case Passive:
                        m_icon = nextIcon;
                        break;
                }

                resetIcon();
            });
        }
    });
}

void StatusNotifierButton::newToolTip()
{
    interface->propertyGetAsync(QLatin1String("ToolTip"), [this] (ToolTip tooltip) {
        QString toolTipTitle = tooltip.title;
        if (!toolTipTitle.isEmpty())
            setToolTip(toolTipTitle);
        else
            interface->propertyGetAsync(QLatin1String("Title"), [this] (QString title) {
                // we should get here only in case the ToolTip.title was empty
                if (!title.isEmpty())
                    setToolTip(title);
            });
    });
}

void StatusNotifierButton::newStatus(QString status)
{
    Status newStatus;
    if (status == QLatin1String("Passive"))
        newStatus = Passive;
    else if (status == QLatin1String("Active"))
        newStatus = Active;
    else
        newStatus = NeedsAttention;

    if (m_status == newStatus)
        return;

    m_status = newStatus;
    resetIcon();
}

void StatusNotifierButton::contextMenuEvent(QContextMenuEvent* /*event*/)
{
    //XXX: avoid showing of parent's context menu, we are (optionally) providing context menu on mouseReleaseEvent
    //QWidget::contextMenuEvent(event);
}

void StatusNotifierButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        interface->Activate(QCursor::pos().x(), QCursor::pos().y());
    else if (event->button() == Qt::MidButton)
        interface->SecondaryActivate(QCursor::pos().x(), QCursor::pos().y());
    else if (Qt::RightButton == event->button()) {
        if (m_menu) {
            m_menu->popup(mapToGlobal(QPoint(0, 0)));
        } else
            interface->ContextMenu(QCursor::pos().x(), QCursor::pos().y());
    }

    QToolButton::mouseReleaseEvent(event);
}

void StatusNotifierButton::wheelEvent(QWheelEvent *event)
{
    interface->Scroll(event->angleDelta().y(), QStringLiteral("vertical"));
}

void StatusNotifierButton::resetIcon()
{
    setIconSize(QSize(16, 16));

    if (m_status == Active && !m_overlayIcon.isNull())
        setIcon(m_overlayIcon);
    else if (m_status == NeedsAttention && !m_attentionIcon.isNull())
        setIcon(m_attentionIcon);
    else if (!m_icon.isNull()) // m_status == Passive
        setIcon(m_icon);
    else if (!m_overlayIcon.isNull())
        setIcon(m_overlayIcon);
    else if (!m_attentionIcon.isNull())
        setIcon(m_attentionIcon);
    else
        setIcon(m_fallbackIcon);
}
