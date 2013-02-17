#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <QMainWindow>

namespace Ui {
class ControlPanel;
}

class ControlPanel : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit ControlPanel(QWidget *parent = 0);
    ~ControlPanel();
    
private slots:
    void on_actionStart_Reflow_triggered();
    void on_actionStop_Reflow_triggered();

private:
    Ui::ControlPanel *ui;
};

#endif // CONTROLPANEL_H
