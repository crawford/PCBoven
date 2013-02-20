#include <QPainter>
#include <QVector>
#include <QtCore/qmath.h>
#include "reflowgraphwidget.h"

#define DEGREES_PER_BAR 10.0
#define SECONDS_PER_BAR 10.0

ReflowGraphWidget::ReflowGraphWidget(QWidget *parent) : QWidget(parent)
{
	_temperatures = new QVector<QPair<QTime, int> >();
	_temperatureTargets = new QVector<QPair<QTime, int> >();
	_maxTime = 0;
	_maxTemperature = 0;
	setContentsMargins(10, 10, 10, 10);
}

void ReflowGraphWidget::setTemperatureTargets(QMap<QTime, int> targets)
{
	_temperatureTargets->clear();
	for (QMap<QTime, int>::const_iterator i = targets.constBegin(); i != targets.constEnd(); i++) {
		_temperatureTargets->append(QPair<QTime, int>(i.key(), i.value()));
		if (i.value() > _maxTemperature)
			_maxTemperature = i.value();
	}
	_maxTime = QTime(0, 0).secsTo(_temperatureTargets->last().first);

	repaint();
}

void ReflowGraphWidget::addTemperature(QTime time, int temperature)
{
	_temperatures->append(QPair<QTime, int>(time, temperature));
	if (temperature > _maxTemperature)
		_maxTemperature = temperature;
	repaint();
}

void ReflowGraphWidget::clearGraph()
{
	_temperatures->clear();
	repaint();
}

void ReflowGraphWidget::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setBackground(QBrush(Qt::white));
	painter.setBackgroundMode(Qt::OpaqueMode);
	painter.fillRect(contentsRect(), painter.background());

	// Draw grid
	painter.setPen(QPen(QBrush(QColor(200, 200, 200)), 1));
	QVector<QLineF> gridLines;
	int divisions = qCeil(_maxTime/SECONDS_PER_BAR);
	for (int i = 0; i <= divisions; i++)
		gridLines.append(QLineF((double)contentsRect().width()/divisions*i + contentsRect().left(),
		                        (double)contentsRect().top(),
		                        (double)contentsRect().width()/divisions*i + contentsRect().left(),
		                        (double)contentsRect().bottom()));
	divisions = qCeil(_maxTemperature/DEGREES_PER_BAR);
	for (int i = 0; i <= divisions; i++)
		gridLines.append(QLineF((double)contentsRect().left(),
		                        (double)contentsRect().height()/divisions*i + contentsRect().top(),
		                        (double)contentsRect().right(),
		                        (double)contentsRect().height()/divisions*i + contentsRect().top()));
	painter.drawLines(gridLines);

	// Draw target temp
	if (!_temperatureTargets->empty()) {
		painter.setPen(QPen(QBrush(QColor(150, 150, 255)), 2));
		QPainterPath patha(QPointF(contentsRect().left(),
		                           contentsRect().bottom() - (double)_temperatureTargets->first().second/_maxTemperature*contentsRect().height()));
		for (int i = 1; i < _temperatureTargets->size(); i++)
			patha.lineTo((double)contentsRect().width()*QTime(0, 0).secsTo(_temperatureTargets->at(i).first)/_maxTime + contentsRect().left(),
			             contentsRect().bottom() - (double)_temperatureTargets->at(i).second/_maxTemperature*contentsRect().height());
		painter.drawPath(patha);
	}

	// Draw actual temp
	if (!_temperatures->empty()) {
		painter.setPen(QPen(QBrush(QColor(200, 0, 0)), 2));
		QPainterPath patha(QPointF(contentsRect().left(),
		                           contentsRect().bottom() - (double)_temperatures->first().second/_maxTemperature*contentsRect().height()));
		for (int i = 1; i < _temperatures->size(); i++)
			patha.lineTo((double)contentsRect().width()*QTime(0, 0).secsTo(_temperatures->at(i).first)/_maxTime + contentsRect().left(),
			             contentsRect().bottom() - (double)_temperatures->at(i).second/_maxTemperature*contentsRect().height());
		painter.drawPath(patha);
	}
}
