#ifndef PLUGINS_H
#define PLUGINS_H

/**
 * Optional initialize function
 * Name: pluginInit
 * @return env
 */
typedef void *(*pluginInitFunc)(
    const std::string &path,
    void *options
);

/**
 * Optional destroy function
 * Name: pluginDone
 * @return env
 */
typedef void(*pluginDoneFunc)(
    void *env
);

/**
 * Mandatory login function
 * Name: pluginLogin
 * @return env
 */
typedef bool(*pluginLoginFunc)(
    void *env,
    const std::string &login,
    const std::string &password
);

class LoginPluginFuncs {
public:
    void *env;
    pluginInitFunc init;
    pluginDoneFunc done;
    pluginLoginFunc login;
};

#endif
