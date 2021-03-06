#include <OpenGL2Writer.hpp>

#ifdef OPENGL_NEW_API
	#include <OpenGL2Window.hpp>
	#include <OpenGL2Widget.hpp>
#else
	#include <OpenGL2OldWidget.hpp>
#endif

#include <VideoFrame.hpp>

#include <QApplication>

OpenGL2Writer::OpenGL2Writer(Module &module) :
	drawable(NULL)
#ifdef OPENGL_NEW_API
	, forceRtt(false)
#endif
{
	addParam("W");
	addParam("H");
	addParam("AspectRatio");
	addParam("Zoom");
	addParam("Flip");
	addParam("Saturation");
	addParam("Brightness");
	addParam("Contrast");
	addParam("Hue");

	SetModule(module);
}
OpenGL2Writer::~OpenGL2Writer()
{
	drawable->deleteMe();
}

bool OpenGL2Writer::set()
{
	bool doReset = false;
#ifdef VSYNC_SETTINGS
	vSync = sets().getBool("VSync");
	if (drawable && !drawable->VSync(vSync))
		doReset = true;
#endif
#ifdef OPENGL_NEW_API
	bool newForceRtt = sets().getBool("ForceRtt");
	if (forceRtt != newForceRtt)
		doReset = true;
	forceRtt = newForceRtt;
#endif
	return !doReset && sets().getBool("Enabled");
}

bool OpenGL2Writer::readyWrite() const
{
	return drawable->isOK;
}

bool OpenGL2Writer::processParams(bool *)
{
	bool doResizeEvent = false;

	const double aspectRatio = getParam("AspectRatio").toDouble();
	const double zoom = getParam("Zoom").toDouble();
	const int flip = getParam("Flip").toInt();
	const float Contrast = (getParam("Contrast").toInt() + 100) / 100.0f;
	const float Saturation = (getParam("Saturation").toInt() + 100) / 100.0f;
	const float Brightness = getParam("Brightness").toInt() / 100.0f;
	const float Hue = getParam("Hue").toInt() / -31.831f;
	if (drawable->aspectRatio != aspectRatio || drawable->zoom != zoom || drawable->flip != flip || drawable->Contrast != Contrast || drawable->Brightness != Brightness || drawable->Saturation != Saturation || drawable->Hue != Hue)
	{
		drawable->zoom = zoom;
		drawable->aspectRatio = aspectRatio;
		drawable->flip = flip;
		drawable->Contrast = Contrast;
		drawable->Brightness = Brightness;
		drawable->Saturation = Saturation;
		drawable->Hue = Hue;
		doResizeEvent = drawable->widget()->isVisible();
	}

	const int outW = getParam("W").toInt();
	const int outH = getParam("H").toInt();
	if (outW > 0 && outH > 0 && (outW != drawable->outW || outH != drawable->outH))
	{
		drawable->outW = outW;
		drawable->outH = outH;

		drawable->clearImg();
		emit QMPlay2Core.dockVideo(drawable->widget());
	}

	if (doResizeEvent)
		drawable->newSize();
	else
		drawable->doReset = true;

	return readyWrite();
}

qint64 OpenGL2Writer::write(const QByteArray &arr)
{
	drawable->isPaused = false;
	VideoFrame::unref(drawable->videoFrameArr);
	drawable->videoFrameArr = arr;
	drawable->updateGL();
	return arr.size();
}
void OpenGL2Writer::writeOSD(const QList< const QMPlay2_OSD * > &osds)
{
	QMutexLocker mL(&drawable->osdMutex);
	drawable->osdList = osds;
}

void OpenGL2Writer::pause()
{
	drawable->isPaused = true;
}

QString OpenGL2Writer::name() const
{
	QString glStr = drawable->glVer ? QString("%1.%2").arg(drawable->glVer / 10).arg(drawable->glVer % 10) : "2";
#ifdef OPENGL_NEW_API
	if (useRtt)
		glStr += " (render-to-texture)";
#endif
#ifdef OPENGL_ES2
	return "OpenGL|ES " + glStr;
#else
	return "OpenGL " + glStr;
#endif
}

bool OpenGL2Writer::open()
{
#ifdef OPENGL_NEW_API
	static const QString platformName = qApp->platformName();
	if (platformName == "wayland" || platformName == "android")
		useRtt = true;
	else
		useRtt = forceRtt;
	if (useRtt)
	{
		foreach (QWidget *w, qApp->allWidgets())
		{
			//Don't use rtt when videoDock has native window
			if (w->objectName() == "videoDock")
			{
				useRtt = !w->internalWinId() || (w == w->window());
				break;
			}
		}
	}
	if (useRtt)
		drawable = new OpenGL2Widget;
	else
		drawable = new OpenGL2Window;
#else
	drawable = new OpenGL2OldWidget;
#endif
	if (drawable->testGL())
	{
#ifdef VSYNC_SETTINGS
		drawable->VSync(vSync);
#endif
		return true;
	}
	return false;
}
