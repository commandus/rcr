#include "login-plugin.h"
#include <algorithm>
#include "utilfile.h"

#define FUNC_NAME_INIT              "pluginInit"
#define FUNC_NAME_DONE              "pluginDone"
#define FUNC_NAME_LOGIN             "pluginLogin"

#ifdef _MSC_VER
#else
#include <dlfcn.h>
#include <algorithm>

#endif

LoginPlugins::LoginPlugins()
{
}

// load plugins
LoginPlugins::LoginPlugins(
    const std::string &pluginDirectory,
    void *options,
    const std::string &suffix
)
{
	load(pluginDirectory, options, suffix);
}

// unload plugins
LoginPlugins::~LoginPlugins()
{
	unload();
}

int LoginPlugins::init(
    const std::string &pluginDirectory,
    void *options
)
{
    for (std::vector<LoginPluginFuncs>::iterator it(funcs.begin()); it != funcs.end(); it++) {
        if (it->init != nullptr)
            it->env = it->init(pluginDirectory, options);
        else
            it->env = nullptr;
    }
    return 0;
}

void LoginPlugins::done()
{
    for (std::vector<LoginPluginFuncs>::const_iterator it(funcs.begin()); it != funcs.end(); it++) {
        if (it->done != nullptr)
            it->done(it->env);
    }
}

bool LoginPlugins::login(
    const std::string &login,
    const std::string &password
)
{
    for (std::vector<LoginPluginFuncs>::const_iterator it(funcs.begin()); it != funcs.end(); it++) {
        bool r = it->login(it->env, login, password);
        if (r)
            return r;
    }
    return false;
}


// load plugin by file name
int LoginPlugins::push(
    const std::string &file
)
{
    HINSTANCE handle;
#ifdef _MSC_VER
    handle = LoadLibraryA(file.c_str());
    if (handle) {
        handles.push_back(handle);

        LoginPluginFuncs fs;
        fs.init = (pluginInitFunc) GetProcAddress(handle, FUNC_NAME_INIT);
        fs.done = (pluginDoneFunc) GetProcAddress(handle, FUNC_NAME_DONE);
        fs.login = (pluginLoginFunc) GetProcAddress(handle, FUNC_NAME_LOGIN);
        if (fs.login) {
            funcs.push_back(fs);
            return 0;
        }
    }
#else
    handle = dlopen(file.c_str(), RTLD_LAZY);
    if (handle) {
        handles.push_back(handle);
        LoginPluginFuncs fs;
        fs.init = (pluginInitFunc) dlsym(handle, FUNC_NAME_INIT);
        fs.done = (pluginDoneFunc) dlsym(handle, FUNC_NAME_DONE);
        fs.login = (pluginLoginFunc) dlsym(handle, FUNC_NAME_LOGIN);
        if (fs.login) {
            funcs.push_back(fs);
            return 0;
        }
    }
#endif
    return -1;
}

// load plugins
int LoginPlugins::load(
	const std::string &pluginDirectory,
    void *options,
    const std::string &suffix
)
{
    std::vector<std::string> files;
    // Load .so files. flags: 0- as is file name, 1- full path
    util::filesInPath(pluginDirectory, suffix, 1, &files);
    // sort alphabetically by file name
    std::sort(files.begin(), files.end());
    int count = 0;
    for (std::vector<std::string>::const_iterator it(files.begin()); it != files.end(); it++) {
        if (push(*it) == 0)
            count++;
    }
    init(pluginDirectory, options);
    return count;   // successfully loaded .so/.dll plugin files
}

void LoginPlugins::unload() {
    done();
    for (std::vector<HINSTANCE>::iterator it(handles.begin()); it != handles.end(); it++) {
#ifdef _MSC_VER
        FreeLibrary(*it);
#else
        dlclose(*it);
#endif
    }
    handles.clear();
}
