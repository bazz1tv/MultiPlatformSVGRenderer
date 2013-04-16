// ============================================================================
// [Include Section]
// ============================================================================
#include <string>


#include <Fog/Core.h>
#include <Fog/G2d.h>

#include <SDL2/SDL.h>
#include "savepng.h"

using namespace std;
// ============================================================================
// [Tools]
// ============================================================================
Fog::TransformF tf(1.0,0.0,0.0, 0.0,1.0,0.0, 0.0,0.0,1.0);
// ----------------------------------------------------------------------------
Uint8 *keys;
float movex,movey;
const int velocity = 8;

static Fog::TransformF createRotationMatrix(Fog::PointF center, float rotation)
{
	Fog::TransformF m;
	Fog::PointF negcenter(-center.getX(),-center.getY());
	
	m.translate(negcenter, Fog::MATRIX_ORDER_APPEND);
	m.rotate(rotation, Fog::MATRIX_ORDER_APPEND);
	m.translate(center, Fog::MATRIX_ORDER_APPEND);
	
	return m;
}

// ============================================================================
// [SdlApplication]
// ============================================================================

// SdlApplication is nothing more than thin wrapper to SDL library. You need
// just to instantiate it and call run() to enter the SDL event loop.
struct SdlApplication
{
	SdlApplication();
	~SdlApplication();
	
	// Application state (just convenience instead of 0, 1, ...).
	enum APP_STATE
	{
		APP_OK = 0,
		APP_FAILED = 1
	};
	
	// Initialize application, called by run(), don't call manually.
	int init(int width, int height);
	
	// Destroy application, called by destructor, don't call manually.
	void destroy();
	
	// Run application, called by your code.
	int run(int width, int height);
	
	// Called to process SDL event.
	void onEvent(SDL_Event* ev);
	
	// Called on timer event.
	static Uint32 _onTimerCb(Uint32 interval, void* param);
	void onTimer();
	
	// Called to render content into buffer.
	void onRender();
	
	// Called to render content using Fog::Painter, called by onRender().
	void onPaint(Fog::Painter *p);
	
	// SDL screen surface, the surface we will paint to.
	SDL_Surface* _screen;
	
	// SDL timer.
	SDL_TimerID _timer;
	
	// Delay between frames.
	int _interval;
	
	// Some stuff we work with.
	double _rotate;
	
	// Whether the application is in event loop.
	bool _running;
	SDL_Window *win;
	
	Fog::SvgDocument svgDocument;
	
	float fpsTotal;
	float fpsCounter;
	Fog::Time fpsTime;
};

SdlApplication::SdlApplication() :
_screen(NULL),
_timer(0),
_interval(100),
_rotate(0.0),
_running(false)
{
	keys = SDL_GetKeyboardState(NULL);
}

SdlApplication::~SdlApplication()
{
	//SDL_DestroyWindow(win);
	destroy();
}

int SdlApplication::init(int width, int height)
{
	// Initialize the SDL library.
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		fprintf(stderr, "SDL_Init() failed: %s\n", SDL_GetError());
		return APP_FAILED;
	}
	
	win = SDL_CreateWindow("SDL2 SVG Renderer -> PNG", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
	_screen = SDL_GetWindowSurface(win);
	// Create main surface.
	if (_screen == NULL)
	{
		fprintf(stderr, "SDL_GetWindowSurface(win) failed: %s\n", SDL_GetError());
		return APP_FAILED;
	}
	
	
	
	// Create timer
	//_timer = SDL_AddTimer(_interval, SdlApplication::_onTimerCb, reinterpret_cast<void*>(this));
	
	// Success.
	return APP_OK;
}

void SdlApplication::destroy()
{
	if (_screen)
	{
		SDL_RemoveTimer(_timer);
		SDL_FreeSurface(_screen);
		SDL_DestroyWindow(win);
		SDL_Quit();
		
		_screen = NULL;
	}
}

int SdlApplication::run(int width, int height)
{
	// Initialize application.
	int state = init(width, height);
	if (state != APP_OK) return state;
	
	// Enter to the SDL event loop.
	SDL_Event ev;
	_running = true;
	
	while (_running == true)
	{
		while (SDL_PollEvent(&ev))
		{
			onEvent(&ev);
			if (_running == false)
			{
				SDL_Surface *ss = SDL_PNGFormatAlpha(_screen);
				SDL_SavePNG(ss,"bla.png");
				SDL_FreeSurface(ss);
				break;
			}
		}
		// This is our timer event.
		if (keys[SDL_SCANCODE_UP])
		{
			tf._22 -= 0.005;
		}
		else if (keys[SDL_SCANCODE_DOWN])
		{
			tf._22 += 0.005;
		}
		if (keys[SDL_SCANCODE_W])
		{
			movey-=velocity;
		}
		else if (keys[SDL_SCANCODE_S])
		{
			movey+=velocity;
		}
		if (keys[SDL_SCANCODE_A])
		{
			movex-=velocity;
		}
		else if (keys[SDL_SCANCODE_D])
		{
			movex+=velocity;
		}
		onTimer();
	}
	return APP_OK;
}

void SdlApplication::onEvent(SDL_Event* ev)
{
	switch (ev->type)
	{
		case SDL_QUIT:
			_running = false;
			break;
			
		case SDL_USEREVENT:
			
			break;
		case SDL_KEYDOWN:
			if (ev->key.keysym.sym == SDLK_ESCAPE)
				_running = false;

			break;
	}
}

Uint32 SdlApplication::_onTimerCb(Uint32 interval, void* param)
{
	// On timer callback is called from different thread, we just send custom event
	// back to the main one.
	//SdlApplication* app = reinterpret_cast<SdlApplication*>(param);
	
	SDL_UserEvent e;
	memset(&e, 0, sizeof(e));
	e.type = SDL_USEREVENT;
	SDL_PushEvent(reinterpret_cast<SDL_Event*>(&e));
	
	return interval;
}

void SdlApplication::onTimer()
{
	// Do some stuff...
	_rotate += 0.01;
	
	// Render new frame.
	onRender();
}

void SdlApplication::onRender()
{
	// Lock surface pixels.
	SDL_LockSurface(_screen);
	
	// Create Fog::Painter instance mapped to the SDL surface data.
	Fog::Painter p;
	
	// Setup image buffer for painter.
	Fog::ImageBits buf;
	
	buf.setData(Fog::SizeI(_screen->w, _screen->h), Fog::IMAGE_FORMAT_XRGB32, _screen->pitch, (reinterpret_cast<uint8_t*>(_screen->pixels)));
	
	// Call our paint handler.
	if (p.begin(buf) == Fog::ERR_OK) onPaint(&p);
	
	// Never forget to call p.end(). Painting can be asynchronous and
	// SDL_UnlockSurface() can invalidate the surface->pixels pointer.
	p.end();
	
	// Unlock surface pixels.
	SDL_UnlockSurface(_screen);
	
	// Flip buffer.
	//SDL_Flip(_screen);
	SDL_UpdateWindowSurface(win);
}
using namespace Fog;
void SdlApplication::onPaint(Fog::Painter *p)
{
	//Time startTime = Time::now();
	
	p->setSource(Argb32(0xFFFFFFFF));
	p->fillAll();
	
	
	PointF pt(movex,movey);
	p->setTransform(tf);
	p->translate(pt);
	
	p->save();
	
	svgDocument.render(p);
	//p->setTransform(tf1);
	
	p->restore();
	
	p->flush(PAINTER_FLUSH_SYNC);
	
	p->resetTransform();
}

// ============================================================================
// [Entry-Point]
// ============================================================================

// SDL may redeclare main to something else, cut it.
//#undef main

int main(int argc, char* argv[])
{
	// ----------------------------------------------------------------------------
	// This makes relative paths work in C++ in Xcode by changing directory to the one right where the app is located
#ifdef __APPLE__
	
	CFURLRef resourcesURL = CFBundleCopyExecutableURL( CFBundleGetMainBundle() );
	char pathtofile[PATH_MAX];
	if (!CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)pathtofile, PATH_MAX))
	{
		// error!
	}
	CFRelease(resourcesURL);
	
	string longpath(pathtofile);
	string desiredpath;
	size_t pos;
	
	pos = longpath.rfind(".app/");
	desiredpath = longpath.substr(0,pos);
	// trim off the rest
	pos = desiredpath.find_last_of('/');
	desiredpath.erase(pos);
	
	chdir(desiredpath.c_str());
#endif
	
	StringW fileName;
	fileName = Ascii8("/Users/bazz/Desktop/heart.svg");
	
	
	
	SdlApplication app;
	
	
	err_t err = app.svgDocument.readFromFile(fileName);
	//app.svgDocument.setDpi(200.0);
	//app.svgDocument.set
	SizeF size = app.svgDocument.getDocumentSize();
	
	/*if (size.w < 200) size.w = 200;
	if (size.h < 200) size.h = 200;
	
	if (size.w > 900) size.w = 900;
	if (size.h > 720) size.h = 720;*/
	size.w = 1440;
	size.h = 900;
	
	float dpi = app.svgDocument.getDpi();
	printf("DPI = %.2f",dpi);
	
	//dpi+=3;
	
	
	
	return app.run((int)size.w,(int)size.h);
}