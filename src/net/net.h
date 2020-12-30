//
//  net.h
//  mkxp-z
//
//  Created by ゾロアーク on 12/29/20.
//

#ifndef net_h
#define net_h

#include <unordered_map>
#include <string>

namespace mkxp_net {

typedef std::unordered_map<std::string, std::string> StringMap;

class HTTPResponse {
public:
    int status();
    std::string &body();
    StringMap &headers();
    ~HTTPResponse();
    
private:
    int _status;
    std::string _body;
    StringMap _headers;
    HTTPResponse();
    
    friend class HTTPRequest;
};

class HTTPRequest {
public:
    HTTPRequest(const char *dest);
    ~HTTPRequest();
    
    StringMap &headers();
    
    std::string destination;
    
    HTTPResponse get();
    HTTPResponse post(StringMap &postData);
    HTTPResponse post(const char *body, const char *content_type);
private:
    StringMap _headers;
};
}

#endif /* net_h */
