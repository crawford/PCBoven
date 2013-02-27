#include <QApplication>
#include <iostream>
#include "controlpanel.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	if (qApp->arguments().count() != 2) {
		std::cerr << "Usage: "
		          << qApp->arguments().first().toUtf8().data()
		          << " reflow-profile"
		          << std::endl;
		return -1;
	}

	ControlPanel w;
	w.show();
	return a.exec();
}

