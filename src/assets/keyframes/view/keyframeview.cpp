/***************************************************************************
 *   Copyright (C) 2011 by Till Theato (root@ttill.de)                     *
 *   Copyright (C) 2017 by Nicolas Carion                                  *
 *   This file is part of Kdenlive (www.kdenlive.org).                     *
 *                                                                         *
 *   Kdenlive is free software: you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Kdenlive is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with Kdenlive.  If not, see <http://www.gnu.org/licenses/>.     *
 ***************************************************************************/

#include "keyframeview.hpp"
#include "assets/keyframes/model/keyframemodellist.hpp"
#include "core.h"
#include "kdenlivesettings.h"

#include <QMouseEvent>
#include <QStylePainter>

#include <KColorScheme>
#include <QFontDatabase>
#include <utility>

KeyframeView::KeyframeView(std::shared_ptr<KeyframeModelList> model, int duration, QWidget *parent)
    : QWidget(parent)
    , m_model(std::move(model))
    , m_duration(duration)
    , m_position(0)
    , m_currentKeyframe(-1)
    , m_currentKeyframeOriginal(-1)
    , m_hoverKeyframe(-1)
    , m_scale(1)
{
    setMouseTracking(true);
    setMinimumSize(QSize(150, 20));
    setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred));
    setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    QPalette p = palette();
    KColorScheme scheme(p.currentColorGroup(), KColorScheme::Window);
    m_colSelected = palette().highlight().color();
    m_colKeyframe = scheme.foreground(KColorScheme::NormalText).color();
    m_size = QFontInfo(font()).pixelSize() * 1.8;
    m_lineHeight = m_size / 2;
    m_offset = m_lineHeight;
    setFixedHeight(m_size);
    connect(m_model.get(), &KeyframeModelList::modelChanged, this, &KeyframeView::slotModelChanged);
}

void KeyframeView::slotModelChanged()
{
    int offset = pCore->getItemIn(m_model->getOwnerId());
    emit atKeyframe(m_model->hasKeyframe(m_position + offset), m_model->singleKeyframe());
    emit modified();
    update();
}

void KeyframeView::slotSetPosition(int pos, bool isInRange)
{
    if (!isInRange) {
        m_position = -1;
        update();
        return;
    }
    if (pos != m_position) {
        m_position = pos;
        int offset = pCore->getItemIn(m_model->getOwnerId());
        emit atKeyframe(m_model->hasKeyframe(pos + offset), m_model->singleKeyframe());
        update();
    }
}

void KeyframeView::initKeyframePos()
{
    emit atKeyframe(m_model->hasKeyframe(m_position), m_model->singleKeyframe());
}

void KeyframeView::slotAddKeyframe(int pos)
{
    if (pos < 0) {
        pos = m_position;
    }
    int offset = pCore->getItemIn(m_model->getOwnerId());
    m_model->addKeyframe(GenTime(size_t(pos + offset), pCore->getCurrentFps()), (KeyframeType)KdenliveSettings::defaultkeyframeinterp());
}

void KeyframeView::slotAddRemove()
{
    int offset = pCore->getItemIn(m_model->getOwnerId());
    if (m_model->hasKeyframe(m_position + offset)) {
        slotRemoveKeyframe(m_position);
    } else {
        slotAddKeyframe(m_position);
    }
}

void KeyframeView::slotEditType(int type, const QPersistentModelIndex &index)
{
    int offset = pCore->getItemIn(m_model->getOwnerId());
    if (m_model->hasKeyframe(m_position + offset)) {
        m_model->updateKeyframeType(GenTime(size_t(m_position + offset), pCore->getCurrentFps()), type, index);
    }
}

void KeyframeView::slotRemoveKeyframe(int pos)
{
    if (pos < 0) {
        pos = m_position;
    }
    int offset = pCore->getItemIn(m_model->getOwnerId());
    m_model->removeKeyframe(GenTime(size_t(pos + offset), pCore->getCurrentFps()));
}

void KeyframeView::setDuration(int dur)
{
    m_duration = dur;
    int offset = pCore->getItemIn(m_model->getOwnerId());
    emit atKeyframe(m_model->hasKeyframe(m_position + offset), m_model->singleKeyframe());
    update();
}

void KeyframeView::slotGoToNext()
{
    if (m_position == m_duration - 1) {
        return;
    }

    bool ok;
    int offset = pCore->getItemIn(m_model->getOwnerId());
    auto next = m_model->getNextKeyframe(GenTime(size_t(m_position + offset), pCore->getCurrentFps()), &ok);

    if (ok) {
        emit seekToPos(qMin((int)next.first.frames(pCore->getCurrentFps()) - offset, m_duration - 1));
    } else {
        // no keyframe after current position
        emit seekToPos(m_duration - 1);
    }
}

void KeyframeView::slotGoToPrev()
{
    if (m_position == 0) {
        return;
    }

    bool ok;
    int offset = pCore->getItemIn(m_model->getOwnerId());
    auto prev = m_model->getPrevKeyframe(GenTime(m_position + offset, pCore->getCurrentFps()), &ok);

    if (ok) {
        emit seekToPos(qMax(0, (int)prev.first.frames(pCore->getCurrentFps()) - offset));
    } else {
        // no keyframe after current position
        emit seekToPos(m_duration - 1);
    }
}

void KeyframeView::mousePressEvent(QMouseEvent *event)
{
    int offset = pCore->getItemIn(m_model->getOwnerId());
    int pos = (event->x() - m_offset) / m_scale;
    if (event->y() < m_lineHeight && event->button() == Qt::LeftButton) {
        bool ok;
        GenTime position(pos + offset, pCore->getCurrentFps());
        auto keyframe = m_model->getClosestKeyframe(position, &ok);
        if (ok && qAbs(keyframe.first.frames(pCore->getCurrentFps()) - pos - offset) * m_scale < ceil(m_lineHeight / 1.5)) {
            m_currentKeyframeOriginal = keyframe.first.frames(pCore->getCurrentFps()) - offset;
            // Select and seek to keyframe
            m_currentKeyframe = m_currentKeyframeOriginal;
            emit seekToPos(m_currentKeyframeOriginal);
            return;
        }
    }

    // no keyframe next to mouse
    m_currentKeyframe = m_currentKeyframeOriginal = -1;
    emit seekToPos(pos);
    update();
}

void KeyframeView::mouseMoveEvent(QMouseEvent *event)
{
    int offset = pCore->getItemIn(m_model->getOwnerId());
    int pos = qBound(0, (int)((event->x() - m_offset) / m_scale), m_duration - 1);
    GenTime position(pos + offset, pCore->getCurrentFps());
    if ((event->buttons() & Qt::LeftButton) != 0u) {
        if (m_currentKeyframe == pos) {
            return;
        }
        if (m_currentKeyframe > 0) {
            if (!m_model->hasKeyframe(pos + offset)) {
                GenTime currentPos(m_currentKeyframe + offset, pCore->getCurrentFps());
                if (m_model->moveKeyframe(currentPos, position, false)) {
                    m_currentKeyframe = pos;
                }
            }
        }
        emit seekToPos(pos);
        return;
    }
    if (event->y() < m_lineHeight) {
        bool ok;
        auto keyframe = m_model->getClosestKeyframe(position, &ok);
        if (ok && qAbs(keyframe.first.frames(pCore->getCurrentFps()) - pos - offset) * m_scale < ceil(m_lineHeight / 1.5)) {
            m_hoverKeyframe = keyframe.first.frames(pCore->getCurrentFps()) - offset;
            setCursor(Qt::PointingHandCursor);
            update();
            return;
        }
    }

    if (m_hoverKeyframe != -1) {
        m_hoverKeyframe = -1;
        setCursor(Qt::ArrowCursor);
        update();
    }
}

void KeyframeView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (m_currentKeyframe >= 0 && m_currentKeyframeOriginal != m_currentKeyframe) {
        int offset = pCore->getItemIn(m_model->getOwnerId());
        GenTime initPos(m_currentKeyframeOriginal + offset, pCore->getCurrentFps());
        GenTime targetPos(m_currentKeyframe + offset, pCore->getCurrentFps());
        bool ok1 = m_model->moveKeyframe(targetPos, initPos, false);
        bool ok2 = m_model->moveKeyframe(initPos, targetPos, true);
        qDebug() << "RELEASING keyframe move" << ok1 << ok2 << initPos.frames(pCore->getCurrentFps()) << targetPos.frames(pCore->getCurrentFps());
    }
}

void KeyframeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->y() < m_lineHeight) {
        int pos = qBound(0, (int)((event->x() - m_offset) / m_scale), m_duration - 1);
        int offset = pCore->getItemIn(m_model->getOwnerId());
        GenTime position(pos + offset, pCore->getCurrentFps());
        bool ok;
        auto keyframe = m_model->getClosestKeyframe(position, &ok);
        if (ok && qAbs(keyframe.first.frames(pCore->getCurrentFps()) - pos - offset) * m_scale < ceil(m_lineHeight / 1.5)) {
            if (keyframe.first.frames(pCore->getCurrentFps()) != offset) {
                m_model->removeKeyframe(keyframe.first);
                if (keyframe.first.frames(pCore->getCurrentFps()) == m_currentKeyframe + offset) {
                    m_currentKeyframe = m_currentKeyframeOriginal = -1;
                }
                if (keyframe.first.frames(pCore->getCurrentFps()) == m_position + offset) {
                    emit atKeyframe(false, m_model->singleKeyframe());
                }
            }
            return;
        }

        // add new keyframe
        m_model->addKeyframe(position, (KeyframeType)KdenliveSettings::defaultkeyframeinterp());
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void KeyframeView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::AltModifier) {
        if (event->angleDelta().y() > 0) {
            slotGoToPrev();
        } else {
            slotGoToNext();
        }
        return;
    }
    int change = event->angleDelta().y() > 0 ? -1 : 1;
    int pos = qBound(0, m_position + change, m_duration - 1);
    emit seekToPos(pos);
}

void KeyframeView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QStylePainter p(this);
    m_scale = (width() - 2 * m_offset) / (double)(m_duration - 1);
    // p.translate(0, m_lineHeight);
    int headOffset = m_lineHeight / 1.5;
    int offset = pCore->getItemIn(m_model->getOwnerId());

    /*
     * keyframes
     */
    for (const auto &keyframe : *m_model.get()) {
        int pos = keyframe.first.frames(pCore->getCurrentFps()) - offset;
        if (pos < 0) continue;
        if (pos == m_currentKeyframe || pos == m_hoverKeyframe) {
            p.setBrush(m_colSelected);
        } else {
            p.setBrush(m_colKeyframe);
        }
        double scaledPos = m_offset + (pos * m_scale);
        p.drawLine(QPointF(scaledPos, headOffset), QPointF(scaledPos, m_lineHeight + headOffset / 2.0));
        switch (keyframe.second.first) {
        case KeyframeType::Linear: {
            QPolygonF position = QPolygonF() << QPointF(-headOffset / 2.0, headOffset / 2.0) << QPointF(0, 0) << QPointF(headOffset / 2.0, headOffset / 2.0)
                                             << QPointF(0, headOffset);
            position.translate(scaledPos, 0);
            p.drawPolygon(position);
            break;
        }
        case KeyframeType::Discrete:
            p.drawRect(QRectF(scaledPos - headOffset / 2.0, 0, headOffset, headOffset));
            break;
        default:
            p.drawEllipse(QRectF(scaledPos - headOffset / 2.0, 0, headOffset, headOffset));
            break;
        }
    }

    p.setPen(palette().dark().color());

    /*
     * Time-"line"
     */
    p.setPen(m_colKeyframe);
    p.drawLine(m_offset, m_lineHeight + (headOffset / 2), width() - m_offset, m_lineHeight + (headOffset / 2));

    /*
     * current position
     */
    if (m_position >= 0 && m_position < m_duration) {
        QPolygon pa(3);
        int cursorwidth = (m_size - (m_lineHeight + headOffset / 2)) / 2 + 1;
        QPolygonF position = QPolygonF() << QPointF(-cursorwidth, m_size) << QPointF(cursorwidth, m_size) << QPointF(0, m_lineHeight + (headOffset / 2) + 1);
        position.translate(m_offset + (m_position * m_scale), 0);
        p.setBrush(m_colKeyframe);
        p.drawPolygon(position);
    }
    p.setOpacity(0.5);
    p.drawLine(m_offset, m_lineHeight, m_offset, m_lineHeight + headOffset);
    p.drawLine(width() - m_offset, m_lineHeight, width() - m_offset, m_lineHeight + headOffset);
}
