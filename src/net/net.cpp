//
//  net.cpp
//  mkxp-z
//
//  Created by ゾロアーク on 12/29/20.
//

#include <stdio.h>

#if defined(MKXPZ_SSL) || defined(MKXPZ_BUILD_XCODE)
#define CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

#include "util/exception.h"

#include "LUrlParser.h"
#include "net.h"

const char* httpErrorNames[] = {
    "Success",
    "Unknown",
    "Connection",
    "Bind IP Address",
    "Read",
    "Write",
    "Exceed Redirect Count",
    "Canceled",
    "SSLConnection",
    "SSL Loading Certs",
    "SSL Server Verification",
    "Unsupported Multipart Boundary Chars"
};

const char *urlErrorNames[] = {
    "OK",
    "Uninitialized",
    "No URL Character",
    "Invalid Scheme Name",
    "No Double Slash",
    "No At Sign",
    "Unexpected End-Of-Line",
    "No Slash"
};

LUrlParser::ParseURL readURL(const char *url) {
    LUrlParser::ParseURL p = LUrlParser::ParseURL::parseURL(std::string(url));
    if (!p.isValid() || p.errorCode_){
        throw Exception(Exception::MKXPError, "Invalid URL: %s", urlErrorNames[p.errorCode_]);
    }
    return p;
}

std::string getHost(LUrlParser::ParseURL url) {
    std::string host;
    host += url.scheme_;
    host += "://";
    host += url.host_;
    
    int port;
    if (!url.port_.empty() && url.getPort(&port)) {
        host += ":";
        host += std::to_string(port);
    }
    return host;
}

std::string getPath(LUrlParser::ParseURL url) {
    std::string path = "/";
    path += url.path_;
    
    if (!url.query_.empty()) {
        path += "?";
        path += url.query_;
    }
    
    return path;
}


using namespace mkxp_net;

HTTPResponse::HTTPResponse() :
    _headers(StringMap()),
    _status(0),
    _body(std::string())
{}

HTTPResponse::~HTTPResponse() {}

std::string &HTTPResponse::body() {
    return _body;
}

StringMap &HTTPResponse::headers() {
    return _headers;
}

int HTTPResponse::status() {
    return _status;
}

HTTPRequest::HTTPRequest(const char *dest) :
    destination(std::string(dest)), _headers(StringMap()) {}

HTTPRequest::~HTTPRequest() {}

StringMap &HTTPRequest::headers() {
    return _headers;
}

HTTPResponse HTTPRequest::get() {
    HTTPResponse ret;
    auto target = readURL(destination.c_str());
    httplib::Client client(getHost(target).c_str());
    httplib::Headers head;
    
    // Seems to need to be disabled for now, at least on macOS
    client.enable_server_certificate_verification(false);
    
    for (auto h : _headers)
        head.emplace(h.first, h.second);
        
    if (auto result = client.Get(getPath(target).c_str(), head)) {
        auto response = result.value();
        ret._status = response.status;
        ret._body = response.body;
        
        for (auto h : response.headers)
            ret._headers.emplace(h.first, h.second);
    }
    else {
        int err = result.error();
        const char *errname = httpErrorNames[err];
        throw Exception(Exception::MKXPError, "Failed to GET %s (%i: %s)", destination.c_str(), err, errname);
    }
    
    return ret;
}

HTTPResponse HTTPRequest::post(StringMap &postData) {
    HTTPResponse ret;
    auto target = readURL(destination.c_str());
    httplib::Client client(getHost(target).c_str());
    httplib::Headers head;
    httplib::Params params;
    
    // Seems to need to be disabled for now, at least on macOS
    client.enable_server_certificate_verification(false);
    
    for (auto h : _headers)
        head.emplace(h.first, h.second);
    
    for (auto p : postData)
        params.emplace(p.first, p.second);
    
    if (auto result = client.Post(getPath(target).c_str(), head, params)) {
        auto response = result.value();
        ret._status = response.status;
        ret._body = response.body;
        
        for (auto h : response.headers)
            ret._headers.emplace(h.first, h.second);
    }
    else {
        int err = result.error();
        const char *errname = httpErrorNames[err];
        throw Exception(Exception::MKXPError, "Failed to POST %s (%i: %s)", destination.c_str(), err, errname);
    }
    return ret;
}

HTTPResponse HTTPRequest::post(const char *body, const char *content_type) {
    HTTPResponse ret;
    auto target = readURL(destination.c_str());
    httplib::Client client(getHost(target).c_str());
    httplib::Headers head;
    
    // Seems to need to be disabled for now, at least on macOS
    client.enable_server_certificate_verification(false);
    
    for (auto h : _headers)
        head.emplace(h.first, h.second);

    if (auto result = client.Post(getPath(target).c_str(), head, body, content_type)) {
        auto response = result.value();
        ret._status = response.status;
        ret._body = response.body;
        
        for (auto h : response.headers)
            ret._headers.emplace(h.first, h.second);
    }
    else {
        int err = result.error();
        throw Exception(Exception::MKXPError, "Failed to POST %s (%i: %s)", destination.c_str(), err, httpErrorNames[err]);
    }
    return ret;
}
