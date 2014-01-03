
#include "plugin.h"
#include "debugwriter.h"
#include "exception.h"

#include "binding-util.h"

static void pluginInit();
static void pluginFini();

Plugin plugin =
{
    pluginInit,
    pluginFini,
    PLUGIN_MRI
};

/* Callable from ruby scripts as 'plugin_function' */
RB_METHOD(pluginFunction)
{
	RB_UNUSED_PARAM;

	const char *str = 0;
	rb_get_args(argc, argv, "|z", &str RB_ARG_END);

	Debug() << "Sample plugin function:" << str;

	return Qnil;
}

void pluginInit()
{
	_rb_define_module_function(rb_mKernel, "plugin_function", pluginFunction);

	Debug() << "Sample plugin inited!";
}

void pluginFini()
{
	Debug() << "Sample plugin finited!";
}
