

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	Visualizer callback;
	ApplicationWin32 app(NULL, 640, 480, "Virtual Scan", GraphicsDeviceTypeD3D11, callback);
	app.messageLoop();

	return 0;
}
