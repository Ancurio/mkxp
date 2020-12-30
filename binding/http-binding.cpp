//
//  http-binding.cpp
//  mkxp-z
//
//  Created by ゾロアーク on 12/29/20.
//

#include <stdio.h>

#include "binding-util.h"

#include "net/net.h"
#include "util/json5pp.hpp"

RB_METHOD(httpGet) {
    RB_UNUSED_PARAM;
    
    VALUE path;
    rb_scan_args(argc, argv, "1", &path);
    SafeStringValue(path);
    
    VALUE ret;

    try {
        mkxp_net::HTTPRequest req(RSTRING_PTR(path));
        auto res = req.get();
        ret = rb_hash_new();
        rb_hash_aset(ret, ID2SYM(rb_intern("status")), INT2NUM(res.status()));
        rb_hash_aset(ret, ID2SYM(rb_intern("body")), rb_str_new_cstr(res.body().c_str()));
        
        VALUE headers = rb_hash_new();
        for (auto header : res.headers()) {
            VALUE key = rb_str_new_cstr(header.first.c_str());
            VALUE val = rb_str_new_cstr(header.second.c_str());
            rb_hash_aset(headers, key, val);
        }
        
        rb_hash_aset(ret, ID2SYM(rb_intern("headers")), headers);
    } catch (Exception &e) {
        raiseRbExc(e);
    }
    
    return ret;
}

RB_METHOD(httpPost) {
    RB_UNUSED_PARAM;
    
    VALUE path, postDataHash;
    rb_scan_args(argc, argv, "2", &path, &postDataHash);
    SafeStringValue(path);
    Check_Type(postDataHash, T_HASH);
    
    VALUE ret;

    try {
        mkxp_net::HTTPRequest req(RSTRING_PTR(path));
        mkxp_net::StringMap postData;
        
        VALUE keys = NUM2INT(rb_funcall(postDataHash, rb_intern("keys"), 0));
        
        for (int i = 0; i < RARRAY_LEN(keys); i++) {
            VALUE k = rb_ary_entry(keys, i);
            VALUE v = rb_hash_aref(postDataHash, k);
            SafeStringValue(k);
            SafeStringValue(v);
            
            postData.emplace(RSTRING_PTR(k), RSTRING_PTR(v));
        }
        
        auto res = req.post(postData);
        ret = rb_hash_new();
        rb_hash_aset(ret, ID2SYM(rb_intern("status")), INT2NUM(res.status()));
        rb_hash_aset(ret, ID2SYM(rb_intern("body")), rb_str_new_cstr(res.body().c_str()));
        
        VALUE headers = rb_hash_new();
        for (auto header : res.headers()) {
            VALUE key = rb_str_new_cstr(header.first.c_str());
            VALUE val = rb_str_new_cstr(header.second.c_str());
            rb_hash_aset(headers, key, val);
        }
        
        rb_hash_aset(ret, ID2SYM(rb_intern("headers")), headers);
    } catch (Exception &e) {
        raiseRbExc(e);
    }
    
    return ret;
}

RB_METHOD(httpPostBody) {
    RB_UNUSED_PARAM;
    
    VALUE path, body, ctype;
    rb_scan_args(argc, argv, "3", &path, &body, &ctype);
    SafeStringValue(path);
    SafeStringValue(body);
    SafeStringValue(ctype);
    
    VALUE ret;

    try {
        mkxp_net::HTTPRequest req(RSTRING_PTR(path));
        mkxp_net::StringMap postData;
        
        auto res = req.post(RSTRING_PTR(body), RSTRING_PTR(ctype));
        ret = rb_hash_new();
        rb_hash_aset(ret, ID2SYM(rb_intern("status")), INT2NUM(res.status()));
        rb_hash_aset(ret, ID2SYM(rb_intern("body")), rb_str_new_cstr(res.body().c_str()));
        
        VALUE headers = rb_hash_new();
        for (auto header : res.headers()) {
            VALUE key = rb_str_new_cstr(header.first.c_str());
            VALUE val = rb_str_new_cstr(header.second.c_str());
            rb_hash_aset(headers, key, val);
        }
        
        rb_hash_aset(ret, ID2SYM(rb_intern("headers")), headers);
    } catch (Exception &e) {
        raiseRbExc(e);
    }
    
    return ret;
}

VALUE json2rb(json5pp::value v) {
    if (v.is_null())
        return Qnil;
    
    if (v.is_number())
        return rb_float_new(v.as_number());
    
    if (v.is_string())
        return rb_str_new_cstr(v.as_string().c_str());
    
    if (v.is_boolean())
        return rb_bool_new(v.as_boolean());
    
    if (v.is_integer())
        return LL2NUM(v.as_integer());
    
    if (v.is_array()) {
        auto &a = v.as_array();
        VALUE ret = rb_ary_new();
        for (auto item : a) {
            rb_ary_push(ret, json2rb(item));
        }
        return ret;
    }
    
    if (v.is_object()) {
        auto &o = v.as_object();
        VALUE ret = rb_hash_new();
        for (auto pair : o) {
            rb_hash_aset(ret, rb_str_new_cstr(pair.first.c_str()), json2rb(pair.second));
        }
        return ret;
    }
}

json5pp::value rb2json(VALUE v) {
    if (v == Qnil)
        return json5pp::value(nullptr);
    
    if (RB_TYPE_P(v, RUBY_T_FLOAT))
        return json5pp::value(RFLOAT_VALUE(v));
    
    if (RB_TYPE_P(v, RUBY_T_STRING))
        return json5pp::value(RSTRING_PTR(v));
    
    if (v == Qtrue || v == Qfalse)
        return json5pp::value(RTEST(v));
    
    if (RB_TYPE_P(v, RUBY_T_FIXNUM))
        return json5pp::value(NUM2DBL(v));
    
    if (RB_TYPE_P(v, RUBY_T_ARRAY)) {
        json5pp::value ret_value = json5pp::array({});
        auto &ret = ret_value.as_array();
        for (int i = 0; i < RARRAY_LEN(v); i++) {
            ret.push_back(rb2json(rb_ary_entry(v, i)));
        }
        return ret_value;
    }
    
    if (RTEST(rb_funcall(v, rb_intern("is_a?"), 1, rb_cHash))) {
        json5pp::value ret_value = json5pp::object({});
        auto &ret = ret_value.as_object();
        
        
        VALUE keys = rb_funcall(v, rb_intern("keys"), 0);
        
        for (int i = 0; i < RARRAY_LEN(keys); i++) {
            VALUE key = rb_ary_entry(keys, i); SafeStringValue(key);
            VALUE val = rb_hash_aref(v, key);
            ret.emplace(RSTRING_PTR(key), rb2json(val));
        }
        
        return ret_value;
    }
    
    raiseRbExc(Exception(Exception::MKXPError, "Invalid value for JSON: %s", RSTRING_PTR(rb_inspect(v))));
}

RB_METHOD(httpJsonParse) {
    RB_UNUSED_PARAM;
    
    VALUE jsonv;
    rb_scan_args(argc, argv, "1", &jsonv);
    SafeStringValue(jsonv);
    
    json5pp::value v;
    try {
        v = json5pp::parse5(RSTRING_PTR(jsonv));
    }
    catch (...) {
        raiseRbExc(Exception(Exception::MKXPError, "Failed to parse JSON"));
    }
    
    return json2rb(v);
}

RB_METHOD(httpJsonStringify) {
    RB_UNUSED_PARAM;
    
    VALUE obj;
    rb_scan_args(argc, argv, "1", &obj);
    
    json5pp::value v = rb2json(obj);
    return rb_str_new_cstr(v.stringify().c_str());
}

void httpBindingInit() {
    VALUE mNet = rb_define_module("HTTPLite");
    _rb_define_module_function(mNet, "get", httpGet);
    _rb_define_module_function(mNet, "post", httpPost);
    _rb_define_module_function(mNet, "post_body", httpPostBody);
    
    VALUE mNetJSON = rb_define_module_under(mNet, "JSON");
    _rb_define_module_function(mNetJSON, "stringify", httpJsonStringify);
    _rb_define_module_function(mNetJSON, "parse", httpJsonParse);
}
