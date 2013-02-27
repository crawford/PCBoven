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

QString ReflowProfile::getTitle()
{
	return _title;
}

QMap<QTime, int> ReflowProfile::getProfile()
{
	return _profile;
}

