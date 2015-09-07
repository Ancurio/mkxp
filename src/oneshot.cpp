#include "oneshot.h"

/******************
 * HERE BE DRAGONS
 ******************/

#include "eventthread.h"
#include "debugwriter.h"

#include <SDL2/SDL.h>

//OS-Specific code
#if defined _WIN32
	#define OS_W32
	#define WIN32_LEAN_AND_MEAN
	#define SECURITY_WIN32
	#include <windows.h>
	#include <security.h>
	#include <shlobj.h>
	#include <SDL2/SDL_syswm.h>
#elif defined __APPLE__
	#define OS_OSX
	#include <pwd.h>
#elif defined __linux__
	#define OS_LINUX
	#include <stdlib.h>
	#include <unistd.h>
	#include <pwd.h>
	#include <dlfcn.h>

	class GtkWidget;

	typedef enum
	{
		GTK_MESSAGE_INFO,
		GTK_MESSAGE_WARNING,
		GTK_MESSAGE_QUESTION,
		GTK_MESSAGE_ERROR
	} GtkMessageType;

	typedef enum
	{
		GTK_BUTTONS_NONE,
		GTK_BUTTONS_OK,
		GTK_BUTTONS_CLOSE,
		GTK_BUTTONS_CANCEL,
		GTK_BUTTONS_YES_NO,
		GTK_BUTTONS_OK_CANCEL
	} GtkButtonsType;

	typedef enum
	{
		GTK_RESPONSE_NONE = -1,
		GTK_RESPONSE_REJECT = -2,
		GTK_RESPONSE_ACCEPT = -3,
		GTK_RESPONSE_DELETE_EVENT = -4,
		GTK_RESPONSE_OK = -5,
		GTK_RESPONSE_CANCEL = -6,
		GTK_RESPONSE_CLOSE = -7,
		GTK_RESPONSE_YES = -8,
		GTK_RESPONSE_NO = -9,
		GTK_RESPONSE_APPLY = -10,
		GTK_RESPONSE_HELP = -11
	} GtkResponseType;
#else
    #error "Operating system not detected."
#endif

struct OneshotPrivate
{
	//Main SDL window
	SDL_Window *window;

	//String data
	std::string lang;
	std::string userName;
	std::string savePath;

	//Dialog text
	std::string txtYes;
	std::string txtNo;

#if defined OS_LINUX
	//GTK+
	void *libgtk;
	void (*gtk_init)(int *argc, char ***argv);
	GtkWidget *(*gtk_message_dialog_new)(void *parent, int flags, GtkMessageType type, GtkButtonsType buttons, const char *message_format, ...);
	void (*gtk_window_set_title)(GtkWidget *window, const char *title);
	GtkResponseType (*gtk_dialog_run)(GtkWidget *dialog);
	void (*gtk_widget_destroy)(GtkWidget *widget);
	void (*gtk_main_quit)();
	void (*gtk_main)();
	unsigned int (*gdk_threads_add_idle)(int (*function)(void *data), void *data);
#endif

	OneshotPrivate()
		: window(0)
#if defined OS_LINUX
		  ,libgtk(0)
#endif
	{
	}

	~OneshotPrivate()
	{
#ifdef OS_LINUX
		if (libgtk)
			dlclose(libgtk);
#endif
	}
};

//OS-SPECIFIC FUNCTIONS
#if defined OS_LINUX
struct linux_DialogData
{
	//Input
	OneshotPrivate *p;
	int type;
	const char *body;
	const char *title;

	//Output
	bool result;
};

static int linux_dialog(void *rawData)
{
	linux_DialogData *data = reinterpret_cast<linux_DialogData*>(rawData);
	OneshotPrivate *p = data->p;

	//Determine correct flags
	GtkMessageType gtktype;
	GtkButtonsType gtkbuttons = GTK_BUTTONS_OK;
	switch (data->type)
	{
	case Oneshot::MSG_INFO:
		gtktype = GTK_MESSAGE_INFO;
		break;
	case Oneshot::MSG_YESNO:
		gtktype = GTK_MESSAGE_QUESTION;
		gtkbuttons = GTK_BUTTONS_YES_NO;
		break;
	case Oneshot::MSG_WARN:
		gtktype = GTK_MESSAGE_WARNING;
		break;
	case Oneshot::MSG_ERR:
		gtktype = GTK_MESSAGE_ERROR;
		break;
	default:
		p->gtk_main_quit();
		return 0;
	}

	//Display dialog and get result
	GtkWidget *dialog = p->gtk_message_dialog_new(0, 0, gtktype, gtkbuttons, data->body);
	p->gtk_window_set_title(dialog, data->title);
	int result = p->gtk_dialog_run(dialog);
	p->gtk_widget_destroy(dialog);

	//Interpret result and return
	data->result = (result == GTK_RESPONSE_OK || result == GTK_RESPONSE_YES);
	p->gtk_main_quit();
	return 0;
}
#elif defined OS_W32
/* Convert WCHAR pointer to std::string */
static std::string w32_fromWide(const WCHAR *ustr)
{
	std::string result;
	int size = WideCharToMultiByte(CP_UTF8, 0, ustr, -1, 0, 0, 0, 0);
	if (size > 0)
	{
		CHAR *str = new CHAR[size];
		if (WideCharToMultiByte(CP_UTF8, 0, ustr, -1, str, size, 0, 0) == size)
			result = str;
		delete [] str;
	}
	return result;
}
/* Convert WCHAR pointer from const char* */
static WCHAR *w32_toWide(const char *str)
{
	if (str)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, 0, 0);
		if (size > 0)
		{
			WCHAR *ustr = new WCHAR[size];
			if (MultiByteToWideChar(CP_UTF8, 0, str, -1, ustr, size) == size)
				return ustr;
			delete [] ustr;
		}
	}

	//Return empty string
	WCHAR *ustr = new WCHAR[1];
	*ustr = 0;
	return ustr;
}
#endif

Oneshot::Oneshot(const RGSSThreadData &threadData)
{
	p = new OneshotPrivate();
	p->window = threadData.window;
	p->savePath = threadData.config.commonDataPath.substr(0, threadData.config.commonDataPath.size() - 1);

	/********************
	 * USERNAME/SAVE PATH
	 ********************/
#if defined OS_W32
	//Get language code
	WCHAR wlang[9];
	GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, wlang, sizeof(wlang) / sizeof(WCHAR));
	p->lang = w32_fromWide(wlang);

	//Get user's name
	ULONG size = 0;
	GetUserNameEx(NameDisplay, 0, &size);
	if (GetLastError() == ERROR_MORE_DATA)
	{
		//Get their full (display) name
		WCHAR *name = new WCHAR[size];
		GetUserNameEx(NameDisplay, name, &size);
		p->userName = w32_fromWide(name);
		delete [] name;
	}
	else
	{
		//Get their login name
		DWORD size2 = 0;
		GetUserName(0, &size2);
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			WCHAR *name = new WCHAR[size2];
			GetUserName(name, &size2);
			p->userName = w32_fromWide(name);
			delete [] name;
		}
	}
#else
	//Get language code
	const char *lc_all = getenv("LC_ALL");
	const char *lang = getenv("LANG");
	const char *code = (lc_all ? lc_all : lang);
	if (code)
	{
		//find first non alphanumeric character, copy language code
		int end = 0;
		for (; code[end] && (code[end] >= 'a' && code[end] <= 'z'); ++end) {}
		p->lang = std::string(code, end);
	}
	else
		p->lang = "en";

	//Get user's name
	struct passwd *pwd = getpwuid(getuid());
	if (pwd)
	{
		if (pwd->pw_gecos && pwd->pw_gecos[0] != ',')
		{
			//Get the user's full name
			int comma = 0;
			for (; pwd->pw_gecos[comma] && pwd->pw_gecos[comma] != ','; ++comma) {}
			p->userName = std::string(pwd->pw_gecos, comma);
		}
		else
			p->userName = pwd->pw_name;
	}
#endif

	/**********
	 * MSGBOX
	 **********/
#ifdef OS_LINUX
#define LOAD_FUNC(name) *reinterpret_cast<void**>(&p->name) = dlsym(p->libgtk, #name)
	//Attempt to link to gtk (prefer gtk2 over gtk3 until I can figure that message box icon out)
	static const char *gtklibs[] =
	{
		"libgtk-x11-2.0.so",
		"libgtk-3.0.so",
	};

	for (size_t i = 0; i < ARRAY_SIZE(gtklibs); ++i)
	{
		if (!(p->libgtk = dlopen("libgtk-x11-2.0.so", RTLD_NOW)))
			p->libgtk = dlopen("libgtk-3.0.so", RTLD_NOW);
		if (p->libgtk)
		{
			//Load functions
			LOAD_FUNC(gtk_init);
			LOAD_FUNC(gtk_message_dialog_new);
			LOAD_FUNC(gtk_window_set_title);
			LOAD_FUNC(gtk_dialog_run);
			LOAD_FUNC(gtk_widget_destroy);
			LOAD_FUNC(gtk_main_quit);
			LOAD_FUNC(gtk_main);
			LOAD_FUNC(gdk_threads_add_idle);

			if (p->gtk_init
					&& p->gtk_message_dialog_new
					&& p->gtk_window_set_title
					&& p->gtk_dialog_run
					&& p->gtk_widget_destroy
					&& p->gtk_main_quit
					&& p->gtk_main
					&& p->gdk_threads_add_idle)
			{
				p->gtk_init(0, 0);
			}
			else
			{
				dlclose(p->libgtk);
				p->libgtk = 0;
			}
		}
		if (p->libgtk)
			break;
	}
#undef LOAD_FUNC
#endif
	/********
	 * MISC
	 ********/
#if defined OS_W32
	//Get windows version
	OSVERSIONINFOW version;
	ZeroMemory(&version, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(version);
	GetVersionEx(&version);
#endif
}

Oneshot::~Oneshot()
{
	delete p;
}

const std::string &Oneshot::lang() const
{
	return p->lang;
}

const std::string &Oneshot::userName() const
{
	return p->userName;
}

const std::string &Oneshot::savePath() const
{
	return p->savePath;
}

void Oneshot::setYesNo(const char *yes, const char *no)
{
	p->txtYes = yes;
	p->txtNo = no;
}

bool Oneshot::msgbox(int type, const char *body, const char *title)
{
#if defined OS_W32
	//Get native window handle
	SDL_SysWMinfo wminfo;
	SDL_version version;
	SDL_VERSION(&version);
	wminfo.version = version;
	SDL_GetWindowWMInfo(p->window, &wminfo);
	HWND hwnd = wminfo.info.win.window;

	//Construct flags
	UINT flags = 0;
	switch (type)
	{
	case MSG_INFO:
		flags = MB_ICONINFORMATION;
		break;
	case MSG_YESNO:
		flags = MB_ICONQUESTION | MB_YESNO;
		break;
	case MSG_WARN:
		flags = MB_ICONWARNING;
		break;
	case MSG_ERR:
		flags = MB_ICONERROR;
		break;
	}

	//Create message box
	WCHAR *wbody = w32_toWide(body);
	WCHAR *wtitle = w32_toWide(title);
	int result = MessageBoxW(hwnd, wbody, wtitle, flags);
	delete [] title;
	delete [] body;

	//Interpret result
	return (result == IDOK || result == IDYES);
#else
#if defined OS_LINUX
	if (p->libgtk)
	{
		linux_DialogData data = {p, type, body, title, 0};
		p->gdk_threads_add_idle(linux_dialog, &data);
		p->gtk_main();
		return data.result;
	}
#endif
	//SDL message box

	//Button data
	static const SDL_MessageBoxButtonData buttonOk = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "OK"};
	static const SDL_MessageBoxButtonData buttonsOk[] = {buttonOk};
	SDL_MessageBoxButtonData buttonYes = {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, p->txtYes.c_str()};
	SDL_MessageBoxButtonData buttonNo = {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, p->txtNo.c_str()};
	SDL_MessageBoxButtonData buttonsYesNo[] = {buttonNo, buttonYes};

	//Messagebox data
	SDL_MessageBoxData data;
	data.window = p->window;
	data.colorScheme = 0;
	data.title = title;
	data.message = body;

	//Set type
	switch (type)
	{
	case MSG_INFO:
	case MSG_YESNO:
		data.flags = SDL_MESSAGEBOX_INFORMATION;
		break;
	case MSG_WARN:
		data.flags = SDL_MESSAGEBOX_WARNING;
		break;
	case MSG_ERR:
		data.flags = SDL_MESSAGEBOX_WARNING;
		break;
	}

	//Set buttons
	switch (type)
	{
	case MSG_INFO:
	case MSG_WARN:
	case MSG_ERR:
		data.numbuttons = 1;
		data.buttons = buttonsOk;
		break;
	case MSG_YESNO:
		data.numbuttons = 2;
		data.buttons = buttonsYesNo;
		break;
	}

	//Show messagebox
	int button;
	SDL_ShowMessageBox(&data, &button);
	return button ? true : false;
#endif
}
