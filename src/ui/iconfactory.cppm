module;

#include <QBrush>
#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRectF>
#include <QString>

export module PlotEngine.UI.IconFactory;

namespace {

QColor softened(const QColor &color)
{
    QColor result = color;
    result.setAlpha(190);
    return result;
}

QPixmap drawIcon(const QString &name, int size, const QColor &accent)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal scale = size / 24.0;
    const QColor line = accent;
    const QColor fill = softened(accent);
    QPen pen(line, 1.8 * scale, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    auto rect = [&](qreal x, qreal y, qreal w, qreal h) {
        return QRectF(x * scale, y * scale, w * scale, h * scale);
    };
    auto lineTo = [&](qreal x1, qreal y1, qreal x2, qreal y2) {
        painter.drawLine(QPointF(x1 * scale, y1 * scale), QPointF(x2 * scale, y2 * scale));
    };
    auto ellipse = [&](qreal x, qreal y, qreal w, qreal h) {
        painter.drawEllipse(rect(x, y, w, h));
    };

    if (name == "new" || name == "add-episode" || name == "add-chapter") {
        painter.drawRoundedRect(rect(5, 3, 11, 16), 1.5 * scale, 1.5 * scale);
        lineTo(8, 7, 13, 7);
        lineTo(8, 10, 13, 10);
        lineTo(17, 14, 22, 14);
        lineTo(19.5, 11.5, 19.5, 16.5);
        if (name == "add-chapter")
            lineTo(8, 13, 13, 13);
    } else if (name == "open" || name == "quick-open") {
        QPainterPath path;
        path.moveTo(3 * scale, 8 * scale);
        path.lineTo(9 * scale, 8 * scale);
        path.lineTo(11 * scale, 10 * scale);
        path.lineTo(21 * scale, 10 * scale);
        path.lineTo(18 * scale, 19 * scale);
        path.lineTo(4 * scale, 19 * scale);
        path.closeSubpath();
        painter.drawPath(path);
        if (name == "quick-open") {
            lineTo(13, 14, 18, 14);
            lineTo(16, 12, 18, 14);
            lineTo(16, 16, 18, 14);
        }
    } else if (name == "recent") {
        ellipse(5, 5, 14, 14);
        lineTo(12, 8, 12, 13);
        lineTo(12, 13, 16, 15);
    } else if (name == "save" || name == "save-as") {
        painter.drawRoundedRect(rect(4, 4, 16, 16), 1.5 * scale, 1.5 * scale);
        painter.drawRect(rect(8, 4, 8, 5));
        painter.drawRoundedRect(rect(8, 14, 8, 6), 1.0 * scale, 1.0 * scale);
        if (name == "save-as") {
            lineTo(16, 10, 21, 10);
            lineTo(19, 8, 21, 10);
            lineTo(19, 12, 21, 10);
        }
    } else if (name == "exit") {
        painter.drawRoundedRect(rect(5, 4, 8, 16), 1.0 * scale, 1.0 * scale);
        lineTo(12, 12, 21, 12);
        lineTo(18, 9, 21, 12);
        lineTo(18, 15, 21, 12);
    } else if (name == "undo" || name == "redo") {
        QPainterPath path;
        if (name == "undo") {
            path.moveTo(10 * scale, 7 * scale);
            path.cubicTo(5 * scale, 7 * scale, 5 * scale, 17 * scale, 14 * scale, 17 * scale);
            lineTo(10, 7, 6, 11);
        } else {
            path.moveTo(14 * scale, 7 * scale);
            path.cubicTo(19 * scale, 7 * scale, 19 * scale, 17 * scale, 10 * scale, 17 * scale);
            lineTo(14, 7, 18, 11);
        }
        painter.drawPath(path);
    } else if (name == "find" || name == "project-search" || name == "search") {
        ellipse(5, 5, 10, 10);
        lineTo(13, 13, 20, 20);
        if (name == "project-search") {
            lineTo(3, 20, 9, 20);
            lineTo(6, 17, 6, 23);
        }
    } else if (name == "replace") {
        ellipse(4, 5, 8, 8);
        lineTo(10, 11, 15, 16);
        lineTo(15, 16, 20, 11);
        lineTo(20, 11, 20, 18);
    } else if (name == "find-next" || name == "find-previous") {
        ellipse(5, 4, 9, 9);
        lineTo(12, 11, 18, 17);
        if (name == "find-next") {
            lineTo(19, 8, 19, 18);
            lineTo(16, 15, 19, 18);
            lineTo(22, 15, 19, 18);
        } else {
            lineTo(19, 18, 19, 8);
            lineTo(16, 11, 19, 8);
            lineTo(22, 11, 19, 8);
        }
    } else if (name == "command-palette") {
        painter.drawRoundedRect(rect(4, 5, 16, 14), 2.0 * scale, 2.0 * scale);
        lineTo(8, 10, 11, 12);
        lineTo(11, 12, 8, 14);
        lineTo(13, 15, 17, 15);
    } else if (name == "ai-settings") {
        painter.setBrush(fill);
        QPainterPath star;
        star.moveTo(12 * scale, 3 * scale);
        star.lineTo(14 * scale, 9 * scale);
        star.lineTo(20 * scale, 12 * scale);
        star.lineTo(14 * scale, 15 * scale);
        star.lineTo(12 * scale, 21 * scale);
        star.lineTo(10 * scale, 15 * scale);
        star.lineTo(4 * scale, 12 * scale);
        star.lineTo(10 * scale, 9 * scale);
        star.closeSubpath();
        painter.drawPath(star);
        painter.setBrush(Qt::NoBrush);
    } else if (name == "ai-rollback") {
        lineTo(7, 7, 3, 11);
        lineTo(3, 11, 7, 15);
        QPainterPath path;
        path.moveTo(4 * scale, 11 * scale);
        path.cubicTo(10 * scale, 4 * scale, 21 * scale, 8 * scale, 18 * scale, 18 * scale);
        painter.drawPath(path);
    } else if (name == "polish") {
        lineTo(6, 18, 16, 8);
        lineTo(14, 6, 18, 10);
        painter.drawEllipse(QPointF(18 * scale, 5 * scale), 1.2 * scale, 1.2 * scale);
        painter.drawEllipse(QPointF(7 * scale, 6 * scale), 0.9 * scale, 0.9 * scale);
        painter.drawEllipse(QPointF(19 * scale, 17 * scale), 0.9 * scale, 0.9 * scale);
    } else if (name == "add-character") {
        ellipse(7, 4, 7, 7);
        QPainterPath body;
        body.moveTo(5 * scale, 20 * scale);
        body.cubicTo(6 * scale, 14 * scale, 15 * scale, 14 * scale, 16 * scale, 20 * scale);
        painter.drawPath(body);
        lineTo(18, 13, 22, 13);
        lineTo(20, 11, 20, 15);
    } else if (name == "add-location") {
        QPainterPath pin;
        pin.moveTo(12 * scale, 21 * scale);
        pin.cubicTo(6 * scale, 14 * scale, 6 * scale, 5 * scale, 12 * scale, 4 * scale);
        pin.cubicTo(18 * scale, 5 * scale, 18 * scale, 14 * scale, 12 * scale, 21 * scale);
        painter.drawPath(pin);
        ellipse(9.5, 7.5, 5, 5);
        lineTo(18, 6, 22, 6);
        lineTo(20, 4, 20, 8);
    } else if (name == "explorer") {
        painter.drawRoundedRect(rect(4, 5, 16, 14), 1.5 * scale, 1.5 * scale);
        lineTo(8, 9, 16, 9);
        lineTo(8, 13, 16, 13);
        lineTo(8, 17, 13, 17);
    } else if (name == "notes") {
        painter.drawRoundedRect(rect(5, 4, 14, 16), 1.5 * scale, 1.5 * scale);
        lineTo(8, 8, 16, 8);
        lineTo(8, 12, 16, 12);
        lineTo(8, 16, 13, 16);
    } else if (name == "editor") {
        painter.drawRoundedRect(rect(4, 5, 16, 14), 1.5 * scale, 1.5 * scale);
        lineTo(8, 9, 17, 9);
        lineTo(8, 13, 15, 13);
        lineTo(8, 17, 18, 17);
    } else if (name == "about") {
        ellipse(5, 5, 14, 14);
        lineTo(12, 11, 12, 16);
        painter.drawPoint(QPointF(12 * scale, 8 * scale));
    } else {
        painter.drawRoundedRect(rect(5, 5, 14, 14), 2.0 * scale, 2.0 * scale);
    }

    return pixmap;
}

}

export namespace PlotEngine::UI {

QIcon icon(const QString &name, const QColor &accent = QColor("#cdd6f4"))
{
    QIcon result;
    for (int size : {16, 20, 24, 32, 48})
        result.addPixmap(drawIcon(name, size, accent));
    return result;
}

}
