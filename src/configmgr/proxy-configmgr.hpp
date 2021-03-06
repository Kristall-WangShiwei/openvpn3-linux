//  OpenVPN 3 Linux client -- Next generation OpenVPN client
//
//  Copyright (C) 2017 - 2018  OpenVPN Inc. <sales@openvpn.net>
//  Copyright (C) 2017 - 2018  David Sommerseth <davids@openvpn.net>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, version 3 of the
//  License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#ifndef OPENVPN3_DBUS_PROXY_CONFIG_HPP
#define OPENVPN3_DBUS_PROXY_CONFIG_HPP

#include <vector>

#include "dbus/core.hpp"

using namespace openvpn;

class OpenVPN3ConfigurationProxy : public DBusProxy {
public:
    OpenVPN3ConfigurationProxy(GBusType bus_type, std::string target)
        : DBusProxy(bus_type,
                    OpenVPN3DBus_name_configuration,
                    OpenVPN3DBus_interf_configuration,
                    "", true)
    {
        std::string object_path = get_object_path(bus_type, target);
        proxy = SetupProxy(OpenVPN3DBus_name_configuration,
                           OpenVPN3DBus_interf_configuration,
                           object_path);
        property_proxy = SetupProxy(OpenVPN3DBus_name_configuration,
                                    "org.freedesktop.DBus.Properties",
                                    object_path);
    }

    OpenVPN3ConfigurationProxy(DBus const & dbusobj, std::string target)
        : DBusProxy(dbusobj,
                    OpenVPN3DBus_name_configuration,
                    OpenVPN3DBus_interf_configuration,
                    "", true)
    {
        std::string object_path = get_object_path(GetBusType(), target);
        proxy = SetupProxy(OpenVPN3DBus_name_configuration,
                           OpenVPN3DBus_interf_configuration,
                           object_path);
        property_proxy = SetupProxy(OpenVPN3DBus_name_configuration,
                                    "org.freedesktop.DBus.Properties",
                                    object_path);
    }

    std::string Import(std::string name, std::string config_blob,
                       bool single_use, bool persistent)
    {
        GVariant *res = Call("Import",
                             g_variant_new("(ssbb)",
                                           name.c_str(),
                                           config_blob.c_str(),
                                           single_use,
                                           persistent));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "Failed to import configuration");
        }

        gchar *buf = NULL;
        g_variant_get(res, "(o)", &buf);
        std::string ret(buf);
        g_variant_unref(res);
        g_free(buf);

        return ret;
    }


    /**
     * Retrieves a string array of configuration paths which are available
     * to the calling user
     *
     * @return A std::vector<std::string> of configuration paths
     */
    std::vector<std::string> FetchAvailableConfigs()
    {
        GVariant *res = Call("FetchAvailableConfigs");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "Failed to retrieve available configurations");
        }
        GVariantIter *cfgpaths = NULL;
        g_variant_get(res, "(ao)", &cfgpaths);

        GVariant *path = NULL;
        std::vector<std::string> ret;
        while ((path = g_variant_iter_next_value(cfgpaths)))
        {
            gsize len;
            ret.push_back(std::string(g_variant_get_string(path, &len)));
            g_variant_unref(path);
        }
        g_variant_unref(res);
        g_variant_iter_free(cfgpaths);
        return ret;
    }


    std::string GetJSONConfig()
    {
        GVariant *res = Call("FetchJSON");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "Failed to retrieve configuration (JSON format)");
        }

        gchar *buf = NULL;
        g_variant_get(res, "(s)", &buf);
        std::string ret(buf);
        g_variant_unref(res);
        g_free(buf);

        return ret;
    }

    std::string GetConfig()
    {
        GVariant *res = Call("Fetch");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy", "Failed to retrieve configuration");
        }

        gchar *buf = NULL;
        g_variant_get(res, "(s)", &buf);
        std::string ret(buf);
        g_variant_unref(res);
        g_free(buf);

        return ret;
    }

    void Remove()
    {
        GVariant *res = Call("Remove");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "Failed to delete the configuration");
        }
    }

    void SetName(std::string name)
    {
        SetProperty("name", name);
    }


    void SetAlias(std::string aliasname)
    {
        SetProperty("alias", aliasname);
    }


    /**
     *  Lock down the configuration profile.  This removes the possibility
     *  for other users than the owner to retrieve the configuration profile
     *  in clear-text or JSON.  The exception is the root user account,
     *  which the openvpn3-service-client process runs as and needs to
     *  be able to retrieve the configuration for the VPN connection.
     *
     * @param lockdown  Boolean flag, if set to true the configuration is
     *                  locked down
     */
    void SetLockedDown(bool lockdown)
    {
        SetProperty("locked_down", lockdown);
    }


    /**
     *  Retrieves the locked-down flag for the configuration profile
     *
     *  @return Returns true if the configuration is locked down.
     */
    bool GetLockedDown()
    {
        return GetBoolProperty("locked_down");
    }


    /**
     *  Manipulate the public-access flag.  When public-access is set to
     *  true, everyone have read access to this configuration profile
     *  regardless of how the access list is configured.
     *
     *  @param public_access Boolean flag.  If set to true, everyone is
     *                       granted read access to the configuration.
     */
    void SetPublicAccess(bool public_access)
    {
        SetProperty("public_access", public_access);
    }


    /**
     *  Retrieve the public-access flag for the configuration profile
     *
     * @return Returns true if public-access is granted
     */
    bool GetPublicAccess()
    {
        return GetBoolProperty("public_access");
    }


    /**
     *  Sets the persist-tun capability setting
     *
     * @param persist_tun  Boolean flag.  If set to true, the persist-tun
     *                     feature is enabled.
     */
    void SetPersistTun(bool persist_tun)
    {
        SetProperty("persist_tun", persist_tun);
    }


    /**
     *   Retrieve the persist-tun capability setting.  If set to true,
     *   the VPN client process should not tear down the tun device upon
     *   reconnections.
     *
     * @return Returns true if persist-tun should be enabled
     */
    bool GetPersistTun()
    {
        return GetBoolProperty("persist_tun");
    }


    void Seal()
    {
        GVariant *res = Call("Seal");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "Failed to seal the configuration");
        }
    }


    /**
     * Grant a user ID (uid) access to this configuration profile
     *
     * @param uid  uid_t value of the user which will be granted access
     */
    void AccessGrant(uid_t uid)
    {
        GVariant *res = Call("AccessGrant", g_variant_new("(u)", uid));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "AccessGrant() call failed");
        }
    }


    /**
     * Revoke the access from a user ID (uid) for this configuration profile
     *
     * @param uid  uid_t value of the user which will get access revoked
     */
    void AccessRevoke(uid_t uid)
    {
        GVariant *res = Call("AccessRevoke", g_variant_new("(u)", uid));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "AccessRevoke() call failed");
        }
    }


    /**
     *  Retrieve the owner UID of this configuration object
     *
     * @return Returns uid_t of the configuration object owner
     */
    uid_t GetOwner()
    {
        return GetUIntProperty("owner");
    }


    /**
     *  Retrieve the complete access control list (acl) for this object.
     *  The acl is essentially just an array of user ids (uid)
     *
     * @return Returns an array if uid_t references for each user granted
     *         access.
     */
    std::vector<uid_t> GetAccessList()
    {
        GVariant *res = GetProperty("acl");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3ConfigurationProxy",
                                "GetAccessList() call failed");
        }
        GVariantIter *acl = NULL;
        g_variant_get(res, "au", &acl);

        GVariant *uid = NULL;
        std::vector<uid_t> ret;
        while ((uid = g_variant_iter_next_value(acl)))
        {
            ret.push_back(g_variant_get_uint32(uid));
            g_variant_unref(uid);
        }
        g_variant_unref(res);
        g_variant_iter_free(acl);
        return ret;
    }


private:
    std::string get_object_path(const GBusType bus_type, std::string target)
    {
        if (target[0] != '/')
        {
            // If the object path does not start with a leadning slash (/),
            // it is an alias, so we need to query /net/openvpn/v3/configuration/aliases
            // to retrieve the proper configuration path
            DBusProxy alias_proxy(bus_type,
                                  OpenVPN3DBus_name_configuration,
                                  OpenVPN3DBus_interf_configuration,
                                  OpenVPN3DBus_rootp_configuration + "/aliases/" + target);
            return alias_proxy.GetStringProperty("config_path");
        }
        else
        {
            return target;
        }
    }

};

#endif // OPENVPN3_DBUS_PROXY_CONFIG_HPP
