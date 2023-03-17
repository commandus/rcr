//
// Created by andrei on 16.03.23.
//

#ifndef RCR_APPSETTINGS_H
#define RCR_APPSETTINGS_H

#include <string>

class AppSettings {
public:
    static const std::string &certificate_ca();
    static const std::string &certificate_server();
    static const std::string &key_server();
};

#endif //RCR_APPSETTINGS_H
