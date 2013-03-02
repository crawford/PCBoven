#ifndef REFLOWPROFILE_H
#define REFLOWPROFILE_H

#include <QString>
#include <QMap>
#include <QTime>

class ReflowProfile
{
	public:
		static ReflowProfile parseFromJson(QByteArray json);

		ReflowProfile();
		ReflowProfile(QString title, QMap<QTime, int> profile);
		void interpolate(int granularity_ms);
		QString getTitle();
		QMap<QTime, int> getProfile();

	private:
		QString _title;
		QMap<QTime, int> _profile;
};

#endif // REFLOWPROFILE_H

