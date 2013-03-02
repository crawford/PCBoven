#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "reflowprofile.h"

ReflowProfile ReflowProfile::parseFromJson(QByteArray json)
{
	QMap<QTime, int> profile;
	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(json, &error);

	QJsonObject obj = doc.object();
	QString title = obj["title"].toString();
	QJsonArray waypoints = obj["waypoints"].toArray();
	QJsonValue node;
	foreach (node, waypoints) {
		QJsonObject waypoint = node.toObject();
		int time = waypoint["timestamp"].toDouble();
		int temp = waypoint["temperature"].toDouble();
		profile.insert(QTime(0, 0).addSecs(time), temp);
	}

	return ReflowProfile(title, profile);
}

ReflowProfile::ReflowProfile()
{
}

ReflowProfile::ReflowProfile(QString title, QMap<QTime, int> profile)
{
	_title = title;
	_profile = profile;
}

void ReflowProfile::interpolate(int granularity_ms)
{
	QMap<QTime, int>::const_iterator thisStep;
	QMap<QTime, int>::const_iterator nextStep;
	QMap<QTime, int> newSteps;

	for (thisStep = _profile.constBegin(), nextStep = thisStep + 1; nextStep != _profile.constEnd(); thisStep++, nextStep++) {
		double tempStep = (double)(nextStep.value() - thisStep.value()) / thisStep.key().msecsTo(nextStep.key());
		for (int t = granularity_ms; t < thisStep.key().msecsTo(nextStep.key()); t += granularity_ms)
			newSteps.insert(thisStep.key().addMSecs(t), thisStep.value() + tempStep*t);
	}

	_profile.unite(newSteps);
}

QString ReflowProfile::getTitle()
{
	return _title;
}

QMap<QTime, int> ReflowProfile::getProfile()
{
	return _profile;
}

