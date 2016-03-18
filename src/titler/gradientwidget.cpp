/*
Copyright (C) 2016  Jean-Baptiste Mardelle <jb@kdenlive.org>
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy 
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gradientwidget.h"

#include <QPainter>
#include <QPixmap>
#include <QtMath>
#include <QLinearGradient>

#include <KSharedConfig>
#include <KConfigGroup>

#include "utils/KoIconUtils.h"

GradientWidget::GradientWidget(QWidget *parent) :
    QDialog(parent),
    Ui::GradientEdit_UI()
{
    setupUi(this);
    updatePreview();
    connect(color1_pos, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
    connect(color2_pos, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
    connect(angle, SIGNAL(valueChanged(int)), this, SLOT(updatePreview()));
    connect(color1, SIGNAL(changed(const QColor &)), this, SLOT(updatePreview()));
    connect(color2, SIGNAL(changed(const QColor &)), this, SLOT(updatePreview()));
    add_gradient->setIcon(KoIconUtils::themedIcon(QStringLiteral("list-add")));
    remove_gradient->setIcon(KoIconUtils::themedIcon(QStringLiteral("list-remove")));
    connect(add_gradient, SIGNAL(clicked()), this, SLOT(saveGradient()));
    connect(remove_gradient, SIGNAL(clicked()), this, SLOT(deleteGradient()));
    QFontMetrics metrics(font());
    m_height = metrics.lineSpacing();
    gradient_list->setIconSize(QSize(6 * m_height, m_height));
    connect(gradient_list, SIGNAL(currentRowChanged(int)), this, SLOT(loadGradient()));
    loadGradients();
}

void GradientWidget::resizeEvent(QResizeEvent* event)
{
    QDialog::resizeEvent(event);
    updatePreview();
}

QString GradientWidget::gradientToString() const
{
    QStringList result;
    result << color1->color().name(QColor::HexArgb) << color2->color().name(QColor::HexArgb) << QString::number(color1_pos->value()) << QString::number(color2_pos->value()) << QString::number(angle->value());
    return result.join(";");
}

QLinearGradient GradientWidget::gradientFromString(const QString &str, int width, int height)
{
    QStringList values = str.split(";");
    QLinearGradient gr;
    if (values.count() < 5) {
        // invalid gradient data
        return gr;
    }
    gr.setColorAt(values.at(2).toDouble() / 100, values.at(0));
    gr.setColorAt(values.at(3).toDouble() / 100, values.at(1));
    double angle = values.at(4).toDouble();
    if (angle <= 90) {
        gr.setStart(0, 0);
        gr.setFinalStop(width * qCos(qDegreesToRadians(angle)), height * qSin(qDegreesToRadians(angle)));
    } else {
        gr.setStart(width, 0);
        gr.setFinalStop(width + width * qCos(qDegreesToRadians(angle)), height * qSin(qDegreesToRadians(angle)));
    }
    return gr;
}

void GradientWidget::updatePreview()
{
    QPixmap p(preview->width(), preview->height());
    m_gradient = QLinearGradient();
    m_gradient.setColorAt(color1_pos->value() / 100.0, color1->color());
    m_gradient.setColorAt(color2_pos->value() / 100.0, color2->color());
    double ang = angle->value();
    if (ang <= 90) {
        m_gradient.setStart(0, 0);
        m_gradient.setFinalStop(p.width() / 2 * qCos(qDegreesToRadians(ang)), p.height() * qSin(qDegreesToRadians(ang)));
    } else {
        m_gradient.setStart(p.width() / 2, 0);
        m_gradient.setFinalStop(p.width() / 2 + (p.width() / 2) * qCos(qDegreesToRadians(ang)), p.height() * qSin(qDegreesToRadians(ang)));
    }
    //qDebug()<<"* * * ANGLE: "<<angle->value()<<" = "<<p.height() * tan(angle->value() * 3.1415926 / 180.0);

    QLinearGradient copy = m_gradient;
    QPointF gradStart = m_gradient.start() + QPointF(p.width() / 2, 0);
    QPointF gradStop = m_gradient.finalStop() + QPointF(p.width() / 2, 0);
    copy.setStart(gradStart);
    copy.setFinalStop(gradStop);
    QBrush br(m_gradient);
    QBrush br2(copy);
    p.fill(Qt::transparent);
    QPainter painter(&p);
    painter.fillRect(0, 0, p.width() / 2, p.height(), br);
    QPainterPath path;
    QFont f = font();
    f.setPixelSize(p.height());
    int margin = p.height() / 8;
    path.addText(p.width() / 2 + 2 * margin, p.height() - margin, f, QStringLiteral("Ax"));
    painter.fillPath(path, br2);
    painter.end();
    preview->setPixmap(p);
}

void GradientWidget::saveGradient()
{
    QPixmap pix(6 * m_height, m_height);
    pix.fill(Qt::transparent);
    m_gradient.setStart(0, pix.height() / 2);
    m_gradient.setFinalStop(pix.width(), pix.height() / 2);
    QPainter painter(&pix);
    painter.fillRect(0, 0, pix.width(), pix.height(), QBrush(m_gradient));
    painter.end();
    QIcon icon(pix);
    int ct = gradient_list->count();
    QStringList existing = getNames();
    QString test = i18n("Gradient %1", ct);
    while (existing.contains(test)) {
        ct++;
        test = i18n("Gradient %1", ct);
    }
    QListWidgetItem *item = new QListWidgetItem(icon, test, gradient_list);
    item->setData(Qt::UserRole, gradientToString());
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

QStringList GradientWidget::getNames() const
{
    QStringList result;
    for (int i = 0; i < gradient_list->count(); i++) {
        result << gradient_list->item(i)->text();
    }
    return result;
}


void GradientWidget::deleteGradient()
{
    QListWidgetItem *item = gradient_list->currentItem();
    if (!item) return;
    delete item;
}

void GradientWidget::loadGradient()
{
    QListWidgetItem *item = gradient_list->currentItem();
    if (!item) return;
    QString data = item->data(Qt::UserRole).toString();
    QStringList res = data.split(';');
    color1->setColor(QColor(res.at(0)));
    color2->setColor(QColor(res.at(1)));
    color1_pos->setValue(res.at(2).toInt());
    color2_pos->setValue(res.at(3).toInt());
    angle->setValue(res.at(4).toInt());
}

QMap <QString, QString> GradientWidget::gradients() const
{
    QMap <QString, QString> gradients;
    for (int i = 0; i < gradient_list->count(); i++) {
        gradients.insert(gradient_list->item(i)->text(), gradient_list->item(i)->data(Qt::UserRole).toString());
    }
    return gradients;
}

QList <QIcon> GradientWidget::icons() const
{
    QList <QIcon> icons;
    for (int i = 0; i < gradient_list->count(); i++) {
        QPixmap pix = gradient_list->item(i)->icon().pixmap(6 * m_height, m_height);
        QIcon icon(pix.scaled(30, 30));
        icons << icon;
    }
    return icons;
}


void GradientWidget::loadGradients()
{
    QMap <QString, QString> gradients;
    gradient_list->clear();
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup group(config, "TitleGradients");
    QMap <QString, QString> values = group.entryMap();
    QMapIterator<QString, QString> k(values);
    while (k.hasNext()) {
        k.next();
        QPixmap pix(6 * m_height, m_height);
        pix.fill(Qt::transparent);
        QLinearGradient gr = gradientFromString(k.value(), pix.width(), pix.height());
        gr.setStart(0, pix.height() / 2);
        gr.setFinalStop(pix.width(), pix.height() / 2);
        QPainter painter(&pix);
        painter.fillRect(0, 0, pix.width(), pix.height(), QBrush(gr));
        painter.end();
        QIcon icon(pix);
        QListWidgetItem *item = new QListWidgetItem(icon, k.key(), gradient_list);
        item->setData(Qt::UserRole, k.value());
        item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }
}
