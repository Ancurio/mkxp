#include "oneshot.h"
#include "etc.h"
#include "sharedstate.h"
#include "binding-util.h"
#include "binding-types.h"

RB_METHOD(oneshotSetYesNo)
{
    RB_UNUSED_PARAM;

    const char *yes;
    const char *no;
    rb_get_args(argc, argv, "zz", &yes, &no RB_ARG_END);
    shState->oneshot().setYesNo(yes, no);
    return Qnil;
}

RB_METHOD(oneshotMsgBox)
{
    RB_UNUSED_PARAM;

    int type;
    const char *body;
    const char *title = 0;
    rb_get_args(argc, argv, "iz|z", &type, &body, &title RB_ARG_END);
    return rb_bool_new(shState->oneshot().msgbox(type, body, title));
}

void oneshotBindingInit()
{
    VALUE module = rb_define_module("Oneshot");
    VALUE msg = rb_define_module_under(module, "Msg");

    //Constants
    rb_const_set(module, rb_intern("USER_NAME"), rb_str_new2(shState->oneshot().userName().c_str()));
    rb_const_set(module, rb_intern("SAVE_PATH"), rb_str_new2(shState->oneshot().savePath().c_str()));
    rb_const_set(module, rb_intern("LANG"), ID2SYM(rb_intern(shState->oneshot().lang().c_str())));
    rb_const_set(msg, rb_intern("INFO"), INT2FIX(Oneshot::MSG_INFO));
    rb_const_set(msg, rb_intern("YESNO"), INT2FIX(Oneshot::MSG_YESNO));
    rb_const_set(msg, rb_intern("WARN"), INT2FIX(Oneshot::MSG_WARN));
    rb_const_set(msg, rb_intern("ERR"), INT2FIX(Oneshot::MSG_ERR));

    //Functions
    _rb_define_module_function(module, "set_yes_no", oneshotSetYesNo);
    _rb_define_module_function(module, "msgbox", oneshotMsgBox);
}
