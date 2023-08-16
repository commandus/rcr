#ifndef PAYLOAD_INSERT_PLUGIN_H_
#define PAYLOAD_INSERT_PLUGIN_H_	1

#include <vector>
#include <string>

#ifdef _MSC_VER
#include <Windows.h>
#define PLUGIN_FILE_NAME_SUFFIX ".dll"
#else
#define PLUGIN_FILE_NAME_SUFFIX ".so"
typedef void * HINSTANCE;
#endif

#include "plugins.h"

class LoginPlugins
{
private:
    std::vector<HINSTANCE> handles;
    std::vector<LoginPluginFuncs> funcs;
    int init(const std::string &pluginDirectory, void *options);
    void done();
protected:
    int push(const std::string &file);
public:
	std::vector <LoginPluginFuncs> plugins;
	LoginPlugins();
	// load plugins
	LoginPlugins(
        const std::string &pluginDirectory,
        void *options,
        const std::string &suffix = PLUGIN_FILE_NAME_SUFFIX
    );
	// unload plugins
	~LoginPlugins();

    bool login(
        const std::string &login,
        const std::string &password
    );
    // load plugins
	int load(
        const std::string &pluginDirectory,
        void *options,
        const std::string &suffix = PLUGIN_FILE_NAME_SUFFIX
    );
	void unload();
};
#endif
