#ifndef PLUGINS_H
#define PLUGINS_H


/**
 * Optional initialize function
 * @return env
 */
typedef void *(*pluginInitFunc)(
    const std::string &path,
    void *options
);

/**
 * Optional destroy function
 * @return env
 */
typedef void(*pluginDoneFunc)(
    void *env
);

/**
 * Optional destroy function
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
