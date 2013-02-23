#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QMainWindow>
#include <QLabel>
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

	private:
		Ui::ControlPanel *ui;
		QLabel *connectionStatus;
		OvenManager *_ovenManager;
		ReflowProfile _profile;
		bool _reflowing;

	private slots:
		void on_actionStart_Reflow_triggered();
		void on_actionStop_Reflow_triggered();
		void ovenConnected();
		void ovenDisconnected();
		void handleError(int error);
};

#endif // CONTROLPANEL_H

