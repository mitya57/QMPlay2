#ifndef OPENGL2OLDWIDGET_HPP
#define OPENGL2OLDWIDGET_HPP

#include <OpenGL2Common.hpp>

#include <QGLWidget>

class OpenGL2OldWidget : public QGLWidget, public OpenGL2Common
{
	Q_OBJECT
public:
	OpenGL2OldWidget();

	QWidget *widget();

	bool testGL();
	bool VSync(bool enable);
	void updateGL();

	void initializeGL();
	void paintGL();
private slots:
	void resetClearCounter();
private:
	void resizeGL(int w, int h);

	bool event(QEvent *e);

	int doClear;
};

#endif // OPENGL2OLDWIDGET_HPP
