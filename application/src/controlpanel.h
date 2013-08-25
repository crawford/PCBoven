#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QMainWindow>
#include <QLabel>
#include <QTime>
#include <QTimer>
#include "ovenmanager.h"
#include "reflowprofile.h"

namespace Ui {
	class ControlPanel;
}

class ControlPanel : public QMainWindow
{
	Q_OBJECT

	public:
		explicit ControlPanel(QWidget *parent = 0);
		~ControlPanel();

		static const int REFLOW_CHECK_PERIOD_MS = 500;
		static const int REFLOW_STEP_PERIOD_MS = 1000;

	private:
		Ui::ControlPanel *ui;
		QLabel *connectionStatus;
		QLabel *reflowStatus;
		OvenManager *_ovenManager;
		ReflowProfile _profile;
		QTime _reflowStartTime;
		QTimer *_reflowTimer;
		QMap<QTime, int>::const_iterator _nextTarget;

	private slots:
		void on_actionStart_Reflow_triggered();
		void on_actionStop_Reflow_triggered();
		void ovenConnected();
		void ovenDisconnected();
		void logReadings(struct oven_state state, QTime timestamp);
		void handleError(QString error);
		void checkProfile();
};

#endif // CONTROLPANEL_H

