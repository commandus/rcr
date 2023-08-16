/**
 * Login using Active directory/LDAP user authentication plugin for rcr service
 * rcr must set -p <plugin-dir> --plugin-options "ldap://ad.acme.com"
 */
#include <sstream>
#include "plugins.h"

#ifdef _MSC_VER
#define EXPORT_PLUGIN_FUNC extern "C" __declspec(dllexport)
#else
#define EXPORT_PLUGIN_FUNC extern "C"
#endif

#include <ldap.h>
#include <ldap_schema.h>

class LDAPSession {
public:
    LDAP *handle;
    std::string ldapServerUrl;
    LDAPSession()
        : handle(nullptr)
    {

    }
    void connect()
    {
        ldap_initialize(&handle, ldapServerUrl.c_str());
    }

    void disconnect()
    {
        ldap_destroy(handle);
        handle = nullptr;
    }
};

EXPORT_PLUGIN_FUNC void *pluginInit(
    const std::string &path,
    void *options
)
{
    LDAPSession *session = new LDAPSession;
    session->ldapServerUrl = *(std::string *) options;
    session->connect();
    return session;
}

extern "C" void pluginDone(
    void *env
)
{
    if (env)
        delete (LDAPSession *) env;
}

extern "C" bool pluginLogin(
    void *env,
    const std::string &login,
    const std::string &password
)
{
    LDAPSession *session = (LDAPSession *) env;
    if (!session->handle)
        return false;
    berval cred;
    cred.bv_val = (char*) password.c_str();
    cred.bv_len = password.length();
    int rc = ldap_sasl_bind_s(session->handle, login.c_str(), LDAP_SASL_SIMPLE, &cred, nullptr, nullptr, nullptr);
    if (rc == 0) {
        ldap_unbind_ext(session->handle, nullptr, nullptr);
    }
    if (!(rc == 0 || rc == 49)) {
        session->disconnect();
        session->connect();
    }
    return rc == LDAP_SUCCESS;
}
