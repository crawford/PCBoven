#ifndef REFLOWGRAPHWIDGET_H
#define REFLOWGRAPHWIDGET_H

#include <QWidget>
#include <QMap>
#include <QTime>
#include <QPair>

class ReflowGraphWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit ReflowGraphWidget(QWidget *parent = 0);
		void setTemperatureTargets(QMap<QTime, int> targets);

	signals:

	public slots:
		void addTemperature(QTime time, int temperature);
		void clearGraph();

	protected:
		virtual void paintEvent(QPaintEvent *);
		QVector<QPair<QTime, int> > *_temperatures;
		QVector<QPair<QTime, int> > *_temperatureTargets;
		unsigned int _maxTime;
		int _maxTemperature;
};

#endif // REFLOWGRAPHWIDGET_H
