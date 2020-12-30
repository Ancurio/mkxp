//
//  http-binding.cpp
//  mkxp-z
//
//  Created by ゾロアーク on 12/29/20.
//

#include <stdio.h>

#include "binding-util.h"

#include "net/net.h"

RB_METHOD(netGet) {
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

RB_METHOD(netPost) {
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

RB_METHOD(netPostBody) {
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

void httpBindingInit() {
    VALUE mNet = rb_define_module("HTTPLite");
    _rb_define_module_function(mNet, "get", netGet);
    _rb_define_module_function(mNet, "post", netPost);
    _rb_define_module_function(mNet, "post_body", netPostBody);
}
