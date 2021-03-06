//  OpenVPN 3 Linux client -- Next generation OpenVPN client
//
//  Copyright (C) 2017      OpenVPN Inc. <sales@openvpn.net>
//  Copyright (C) 2017      David Sommerseth <davids@openvpn.net>
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

/**
 * @file   proxy-sessionmgr.hpp
 *
 * @brief  Provides a C++ object implementation of a D-Bus session manger
 *         and session object.  This proxy class will perform the D-Bus
 *         calls and provide the results as native C++ data types.
 */


#ifndef OPENVPN3_DBUS_PROXY_SESSION_HPP
#define OPENVPN3_DBUS_PROXY_SESSION_HPP

#include <iostream>

#include "dbus/core.hpp"
#include "dbus/requiresqueue-proxy.hpp"
#include "client/statistics.hpp"
#include "client/backendstatus.hpp"
#include "log/log-helpers.hpp"

using namespace openvpn;


/**
 * Carries a log event record as reported by a VPN backend client
 */
struct LogEvent
{
    LogEvent()
    {
        reset();
    }


    LogEvent(GVariant *status)
    {
        reset();
        Parse(status);
    }

    void reset()
    {
        group = LogGroup::UNDEFINED;
        group_str = "";
        category = LogCategory::UNDEFINED;
        category_str = "";
        message = "";
    }

    void Parse(GVariant *logevent)
    {
        GVariant *d = nullptr;
        unsigned int v = 0;

        d = g_variant_lookup_value(logevent, "log_group", G_VARIANT_TYPE_UINT32);
        v = g_variant_get_uint32(d);
        if (v > 0 && v < LogGroup_str.size())
        {
            group = (LogGroup) v;
            group_str = std::string(LogGroup_str[v]);
        }
        g_variant_unref(d);

        d = g_variant_lookup_value(logevent, "log_category", G_VARIANT_TYPE_UINT32);
        v = g_variant_get_uint32(d);
        if (v > 0 && v < LogCategory_str.size())
        {
            category = (LogCategory) v;
            category_str = std::string(LogCategory_str[v]);
        }
        g_variant_unref(d);

        gsize len;
        d = g_variant_lookup_value(logevent,
                                   "log_message", G_VARIANT_TYPE_STRING);
        message = std::string(g_variant_get_string(d, &len));
        g_variant_unref(d);
        if (len != message.size())
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "Failed retrieving log event message text "
                                "(inconsistent length)");
        }
    }

    LogGroup group;
    std::string group_str;
    LogCategory category;
    std::string category_str;
    std::string message;
};


/**
 * This exception is thrown when the OpenVPN3SessionProxy::Ready() call
 * indicates the VPN backend client needs more information from the
 * frontend process.
 */
class ReadyException : public DBusException
{
public:
    ReadyException(const std::string& err, const char *filen,
                   const unsigned int linenum, const char *fn) noexcept
        : DBusException("ReadyException", err, filen, linenum, fn)
    {
    }


    virtual ~ReadyException() throw()
    {
    }


    virtual const char* what() const throw()
    {
        return std::string("[ReadyException]" + errorstr).c_str();
    }

};
#define THROW_READYEXCEPTION(fault_data) throw ReadyException(fault_data, __FILE__, __LINE__, __FUNCTION__)


/**
 *  Client proxy implementation interacting with a
 *  SessionObject in the session manager over D-Bus
 */
class OpenVPN3SessionProxy : public DBusRequiresQueueProxy
{
public:
    /**
     * Initilizes the D-Bus client proxy.  This constructor will establish
     * the D-Bus connection by itself.
     *
     * @param bus_type   Defines if the connection is on the system or session
     *                   bus.
     * @param objpath    D-Bus object path to the SessionObjectes
     */
    OpenVPN3SessionProxy(GBusType bus_type, std::string objpath)
        : DBusRequiresQueueProxy(bus_type,
                                 OpenVPN3DBus_name_sessions,
                                 OpenVPN3DBus_interf_sessions,
                                 objpath,
                                 "UserInputQueueGetTypeGroup",
                                 "UserInputQueueFetch",
                                 "UserInputQueueCheck",
                                 "UserInputProvide")
    {
    }

    /**
     * Initilizes the D-Bus client proxy.  This constructor will use an
     * existing D-Bus connection object for all D-Bus calls
     *
     * @param dbusobj    DBus connection object
     * @param objpath    D-Bus object path to the SessionObject
     */
    OpenVPN3SessionProxy(DBus & dbusobj, const std::string objpath)
        : DBusRequiresQueueProxy(dbusobj,
                                 OpenVPN3DBus_name_sessions,
                                 OpenVPN3DBus_interf_sessions,
                                 objpath,
                                 "UserInputQueueGetTypeGroup",
                                 "UserInputQueueFetch",
                                 "UserInputQueueCheck",
                                 "UserInputProvide")
    {
    }


    /**
     *  Only valid if the session object path points at the main session
     *  manager object.  This starts a new VPN backend client process, running
     *  with the needed privileges.
     *
     * @param cfgpath  VPN profile configuration D-Bus path to use for the
     *                 backend client
     * @return Returns a D-Bus object path string to the session object
     *         created
     */
    std::string NewTunnel(std::string cfgpath)
    {
        GVariant *res = Call("NewTunnel",
                             g_variant_new("(o)", cfgpath.c_str()));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "Failed to start a new tunnel");
        }

        gchar *buf = NULL;
        g_variant_get(res, "(o)", &buf);
        std::string ret(buf);
        g_variant_unref(res);
        g_free(buf);

        return ret;
    }


    /**
     * Retrieves an array of strings with session paths which are available
     * to the calling user
     *
     * @return A std::vector<std::string> of session paths
     */
    std::vector<std::string> FetchAvailableSessions()
    {
        GVariant *res = Call("FetchAvailableSessions");
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "Failed to retrieve available sessions");
        }
        GVariantIter *sesspaths = NULL;
        g_variant_get(res, "(ao)", &sesspaths);

        GVariant *path = NULL;
        std::vector<std::string> ret;
        while ((path = g_variant_iter_next_value(sesspaths)))
        {
            gsize len;
            ret.push_back(std::string(g_variant_get_string(path, &len)));
            g_variant_unref(path);
        }
        g_variant_unref(res);
        g_variant_iter_free(sesspaths);
        return ret;
    }


    /**
     *  Makes the VPN backend client process start the connecting to the
     *  VPN server
     */
    void Connect()
    {
        simple_call("Connect", "Failed to start a new tunnel");
    }

    /**
     *  Makes the VPN backend client process disconnect and then
     *  instantly reconnect to the VPN server
     */
    void Restart()
    {
        simple_call("Restart", "Failed to restart tunnel");
    }


    /**
     *  Disconnects and shuts down the VPN backend process.  This call
     *  will invalidate the current session object.  This can also be used
     *  to shutdown a backend process before doing a Connect() call.
     */
    void Disconnect()
    {
        simple_call("Disconnect", "Failed to disconnect tunnel");
    }


    /**
     *  Pause an on-going VPN tunnel.  Pausing and Resuming an existing VPN
     *  tunnel is generally much faster than doing a full Disconnect() and
     *  Connect() cycle.
     *
     * @param reason  A string provided to the VPN backend process why the
     *                tunnel was suspended.  Primarily used for logging.
     */
    void Pause(std::string reason)
    {
        GVariant *res = Call("Pause",
                             g_variant_new("(s)", reason.c_str()));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "Failed to pause tunnel");
        }
    }


    /**
     *   Resumes a paused VPN tunnel
     */
    void Resume()
    {
        simple_call("Resume", "Failed to resume tunnel");
    }


    /**
     *  Checks if the VPN backend process has all it needs to start connecting
     *  to a VPN server.  If it needs more information from the front-end, a
     *  ReadyException will be thrown with more details.
     */
    void Ready()
    {
        try
        {
            simple_call("Ready", "Connection not ready to connect yet");
        }
        catch (DBusException& excp)
        {
            THROW_READYEXCEPTION(excp.getRawError());
        }
    }


    /**
     * Retrieves the last reported status from the VPN backend
     *
     * @return  Returns a populated struct BackendStatus with the full status.
     */
    BackendStatus GetLastStatus()
    {
        GVariant *status = GetProperty("status");
        BackendStatus ret(status);
        g_variant_unref(status);
        return ret;
    }


    /**
     *  Will the VPN client backend send log messages via
     *  the session manager?
     *
     * @return  Returns true if session manager will proxy log events from
     *          the VPN client backend
     */
    bool GetReceiveLogEvents()
    {
        return GetBoolProperty("receive_log_events");

    }


    /**
     *  Change the session manager log event proxy
     *
     * @param enable  If true, the session manager will proxy log events
     */
    void SetReceiveLogEvents(bool enable)
    {
        SetProperty("receive_log_events", enable);
    }


    /**
     *  Get the log verbosity of the log messages being proxied
     *
     * @return  Returns an integer between 0 and 6,
     *          where 6 is the most verbose.  With 0 only fatal and critical
     *          errors will be provided
     */
    unsigned int GetLogVerbosity()
    {
        return GetUIntProperty("log_verbosity");
    }


    /**
     *  Sets the log verbosity level of proxied log events
     *
     * @param loglevel  An integer between 0 and 6, where 6 is the most
     *                  verbose and 0 the least.
     */
    void SetLogVerbosity(unsigned int loglevel)
    {
        SetProperty("log_verbosity", loglevel);
    }


    /**
     * Retrieve the last log event which has been saved
     *
     * @return Returns a populated struct LogEvent with the the complete log
     *         event.
     */
    LogEvent GetLastLogEvent()
    {
        GVariant *logev = GetProperty("last_log");
        LogEvent ret(logev);
        g_variant_unref(logev);
        return ret;
    }

    /**
     * Retrieves statistics of a running VPN tunnel.  It is gathered by
     * retrieving the 'statistics' session object property.
     *
     * @return Returns a ConnectionStats (std::vector<ConnectionStatDetails>)
     *         array of all gathered statistics.
     */
    ConnectionStats GetConnectionStats()
    {
        GVariant * statsprops = GetProperty("statistics");
        GVariantIter * stats_ar = nullptr;
        g_variant_get(statsprops, "a{sx}", &stats_ar);

        ConnectionStats ret;
        GVariant * r = nullptr;
        while ((r = g_variant_iter_next_value(stats_ar)))
        {
            gchar *key = nullptr;
            gint64 val;
            g_variant_get(r, "{sx}", &key, &val);
            ret.push_back(ConnectionStatDetails(std::string(key), val));
            g_variant_unref(r);
        }
        return ret;
    }


    /**
     *  Manipulate the public-access flag.  When public-access is set to
     *  true, everyone have access to this session regardless of how the
     *  access list is configured.
     *
     *  @param public_access Boolean flag.  If set to true, everyone is
     *                       granted read access to the session.
     */
    void SetPublicAccess(bool public_access)
    {
        SetProperty("public_access", public_access);
    }


    /**
     *  Retrieve the public-access flag for session
     *
     * @return Returns true if public-access is granted
     */
    bool GetPublicAccess()
    {
        return GetBoolProperty("public_access");
    }


    /**
     * Grant a user ID (uid) access to this session
     *
     * @param uid  uid_t value of the user which will be granted access
     */
    void AccessGrant(uid_t uid)
    {
        GVariant *res = Call("AccessGrant", g_variant_new("(u)", uid));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "AccessGrant() call failed");
        }
    }


    /**
     * Revoke the access from a user ID (uid) for this session
     *
     * @param uid  uid_t value of the user which will get access revoked
     */
    void AccessRevoke(uid_t uid)
    {
        GVariant *res = Call("AccessRevoke", g_variant_new("(u)", uid));
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                "AccessRevoke() call failed");
        }
    }


    /**
     *  Retrieve the owner UID of this session object
     *
     * @return Returns uid_t of the session object owner
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
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
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
    /**
     * Simple wrapper for simple D-Bus method calls not requiring much
     * input.  Will also throw a DBusException in case of errors.
     *
     * @param method  D-Bus method to call
     * @param errstr  Error string to provide to the user in case of failures
     */
    void simple_call(std::string method, std::string errstr)
    {
        GVariant *res = Call(method);
        if (NULL == res)
        {
            THROW_DBUSEXCEPTION("OpenVPN3SessionProxy",
                                errstr);
        }
    }

};

#endif // OPENVPN3_DBUS_PROXY_CONFIG_HPP
