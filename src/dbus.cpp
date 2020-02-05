#include <cstdio>
#include <iostream>
#include <sstream>
#include <array>
#include "dbus_info.h"

using ms = std::chrono::milliseconds;

struct metadata spotify;

std::string format_signal(const DBusSignal& s)
{
    std::stringstream ss;
    ss << "type='signal',interface='" << s.intf << "'";
    ss << ",member='" << s.signal << "'";
    return ss.str();
}

void get_string_array(DBusMessage *msg, std::vector<std::string>& entries)
{
    DBusMessageIter iter, subiter;
    dbus_message_iter_init (msg, &iter);
    int current_type = DBUS_TYPE_INVALID;

    current_type = dbus_message_iter_get_arg_type (&iter);

    if (current_type != DBUS_TYPE_ARRAY) {
        std::cerr << "Not an array :" << (char)current_type << std::endl;
        return;
    }

    char *val = nullptr;

    dbus_message_iter_recurse (&iter, &subiter);
    while ((current_type = dbus_message_iter_get_arg_type (&subiter)) != DBUS_TYPE_INVALID) {
        if (current_type == DBUS_TYPE_STRING)
        {
            dbus_message_iter_get_basic (&subiter, &val);
            entries.push_back(val);
        }
        dbus_message_iter_next (&subiter);
    }
}

void parse_property_changed(DBusMessage *msg, std::string& source, std::vector<std::pair<std::string, std::string>>& entries)
{
    char *name = nullptr;
    int i;
    double d;

    std::vector<DBusMessageIter> stack;
    stack.push_back({});

#ifndef NDEBUG
    std::vector<char> tabs;
    tabs.push_back('\0');
#endif

    dbus_message_iter_init (msg, &stack.back());
    int type, prev_type = 0;

    type = dbus_message_iter_get_arg_type (&stack.back());
    if (type != DBUS_TYPE_STRING) {
        std::cerr << __func__ << "First element is not a string" << std::endl;
        return;
    }

    dbus_message_iter_get_basic(&stack.back(), &name);
    source = name;
#ifndef NDEBUG
    std::cout << name << std::endl;
#endif

    std::pair<std::string, std::string> kv;

    dbus_message_iter_next (&stack.back());
    while ((type = dbus_message_iter_get_arg_type (&stack.back())) != DBUS_TYPE_INVALID) {
#ifndef NDEBUG
        tabs.back() = ' ';
        tabs.resize(stack.size() + 1, ' ');
        tabs.back() = '\0';
        std::cout << tabs.data() << "Type: " << (char)type;
#endif

        if (type == DBUS_TYPE_STRING) {
            dbus_message_iter_get_basic(&stack.back(), &name);
#ifndef NDEBUG
            std::cout << "=" << name << std::endl;
#endif
            if (prev_type == DBUS_TYPE_DICT_ENTRY) // is key ?
                kv.first = name;
            if (prev_type == DBUS_TYPE_VARIANT || prev_type == DBUS_TYPE_ARRAY) { // is value ?
                kv.second = name;
                entries.push_back(kv);
            }
        }
        else if (type == DBUS_TYPE_INT32) {
            dbus_message_iter_get_basic(&stack.back(), &i);
#ifndef NDEBUG
            std::cout << "=" << i << std::endl;
#endif
        }
        else if (type == DBUS_TYPE_DOUBLE) {
            dbus_message_iter_get_basic(&stack.back(), &d);
#ifndef NDEBUG
            std::cout << "=" << d << std::endl;
#endif
        }
        else if (type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_DICT_ENTRY || type == DBUS_TYPE_VARIANT) {
#ifndef NDEBUG
            std::cout << std::endl;
#endif
            prev_type = type;
            DBusMessageIter iter;
            dbus_message_iter_recurse (&stack.back(), &iter);
            if (dbus_message_iter_get_arg_type (&stack.back()) != DBUS_TYPE_INVALID)
                stack.push_back(iter);
            continue;
        } else {
#ifndef NDEBUG
            std::cout << std::endl;
#endif
        }

        while(FALSE == dbus_message_iter_next (&stack.back()) && stack.size() > 1) {
            stack.pop_back();
            prev_type = 0;
        }
    }
}

void get_dict_string_array(DBusMessage *msg, std::vector<std::pair<std::string, std::string>>& entries)
{
    DBusMessageIter iter, subiter;
    dbus_message_iter_init (msg, &iter);
    int current_type = DBUS_TYPE_INVALID;

    current_type = dbus_message_iter_get_arg_type (&iter);

    if (current_type == DBUS_TYPE_VARIANT) {
        dbus_message_iter_recurse (&iter, &subiter);
        current_type = dbus_message_iter_get_arg_type (&subiter);
        iter = subiter;
    }

    if (current_type != DBUS_TYPE_ARRAY) {
        std::cerr << "Not an array " << (char)current_type << std::endl;
        return;
    }

    char *val_key = nullptr, *val_value = nullptr;

    dbus_message_iter_recurse (&iter, &subiter);
    while ((current_type = dbus_message_iter_get_arg_type (&subiter)) != DBUS_TYPE_INVALID) {
        // printf("type: %d\n", current_type);

        if (current_type == DBUS_TYPE_DICT_ENTRY)
        {
            dbus_message_iter_recurse (&subiter, &iter);

            // dict entry key
            // printf("\tentry: {%c, ", dbus_message_iter_get_arg_type (&iter));
            dbus_message_iter_get_basic (&iter, &val_key);
            std::string key = val_key;

            // dict entry value
            dbus_message_iter_next (&iter);

            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT)
                dbus_message_iter_recurse (&iter, &iter);

            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse (&iter, &iter);
                if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
                    dbus_message_iter_get_basic (&iter, &val_value);
                    if (key.find("artist") != std::string::npos)
                        spotify.artists.insert(spotify.artists.begin(), val_value);
                }
            }

            // printf("%c}\n", dbus_message_iter_get_arg_type (&iter));
            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic (&iter, &val_value);
                if (key.find("title") != std::string::npos)
                    spotify.title = val_value;

                if (key.find("artUrl") != std::string::npos)
                    spotify.artUrl = val_value;
                entries.push_back({val_key, val_value});
            }
        }
        dbus_message_iter_next (&subiter);
    }

    if (spotify.artists.size() || !spotify.title.empty())
        spotify.valid = true;
}

void get_spotify_metadata(dbusmgr::dbus_manager& dbus)
{
    spotify.artists.clear();

    DBusError error;
    ::dbus_error_init(&error);

    DBusMessage * dbus_reply = nullptr;
    DBusMessage * dbus_msg = nullptr;

    if (nullptr == (dbus_msg = ::dbus_message_new_method_call("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get"))) {
       throw std::runtime_error("unable to allocate memory for dbus message");
    }

    //mah pointers
    const char *v_STRINGS[] = {
        "org.mpris.MediaPlayer2.Player",
        "Metadata",
    };

    if (!dbus_message_append_args (dbus_msg, DBUS_TYPE_STRING, &v_STRINGS[0], DBUS_TYPE_STRING, &v_STRINGS[1], DBUS_TYPE_INVALID)) {
        ::dbus_message_unref(dbus_msg);
        throw std::runtime_error(error.message);
    }

    if (nullptr == (dbus_reply = ::dbus_connection_send_with_reply_and_block(dbus.get_conn(), dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &error))) {
        ::dbus_message_unref(dbus_msg);
        throw dbusmgr::dbus_error(&error);
    } else {

        std::vector<std::pair<std::string, std::string>> entries;
        get_dict_string_array(dbus_reply, entries);

        // std::cout << "String entries:\n";
        //for(auto& p : entries) {
        //    std::cout << p.first << std::endl;
        //}
    }

    ::dbus_message_unref(dbus_msg);
    ::dbus_message_unref(dbus_reply);
    ::dbus_error_free(&error);
}

namespace dbusmgr {
void dbus_manager::init()
{
    ::dbus_threads_init_default();

    if ( nullptr == (m_dbus_conn = ::dbus_bus_get(DBUS_BUS_SESSION, &m_error)) ) {
        throw dbus_error(&m_error);
    }
    std::cout << "Connected to D-Bus as \"" << ::dbus_bus_get_unique_name(m_dbus_conn) << "\"." << std::endl;

    try {
        std::lock_guard<std::mutex> lk(spotify.mutex);
        get_spotify_metadata(*this);
    } catch (dbus_error& e) {
        std::cerr << "Failed to get initial Spotify metadata: " << e.what() << std::endl;
    }
    connect_to_signals();
    m_inited = true;
}

dbus_manager::~dbus_manager()
{
    // unreference system bus connection instead of closing it
    if (m_dbus_conn) {
        disconnect_from_signals();
        ::dbus_connection_unref(m_dbus_conn);
        m_dbus_conn = nullptr;
    }
    ::dbus_error_free(&m_error);
}

void dbus_manager::connect_to_signals()
{
    for (auto kv : m_signals) {
        auto signal = format_signal(kv);
        ::dbus_bus_add_match(m_dbus_conn, signal.c_str(), &m_error);
        if (::dbus_error_is_set(&m_error)) {
            ::perror(m_error.name);
            ::perror(m_error.message);
            ::dbus_error_free(&m_error);
            //return;
        }
    }

    start_thread();
}

void dbus_manager::disconnect_from_signals()
{
    for (auto kv : m_signals) {
        auto signal = format_signal(kv);
        ::dbus_bus_remove_match(m_dbus_conn, signal.c_str(), &m_error);
        if (dbus_error_is_set(&m_error)) {
            ::perror(m_error.name);
            ::perror(m_error.message);
            ::dbus_error_free(&m_error);
        }
    }

    stop_thread();
}

void dbus_manager::add_callback(CBENUM type, callback_func func)
{
    m_callbacks[type] = func;
}

void dbus_manager::stop_thread()
{
    m_quit = true;
    if (m_thread.joinable())
        m_thread.join();
}

void dbus_manager::start_thread()
{
    stop_thread();
    m_quit = false;
    m_thread = std::thread(dbus_thread, this);
}

void dbus_manager::dbus_thread(dbus_manager *pmgr)
{
    DBusError error;
    DBusMessage *msg = nullptr;

    ::dbus_error_init(&error);

    // loop listening for signals being emmitted
    while (!pmgr->m_quit) {

        // non blocking read of the next available message
        if (!::dbus_connection_read_write(pmgr->m_dbus_conn, 0))
            return; // connection closed

        msg = ::dbus_connection_pop_message(pmgr->m_dbus_conn);

        // loop again if we haven't read a message
        if (nullptr == msg) {
            std::this_thread::sleep_for(ms(10));
            continue;
        }

        for (auto& sig : pmgr->m_signals) {
            if (::dbus_message_is_signal(msg, sig.intf, sig.signal))
            {

#ifndef NDEBUG
                std::cerr << __func__ << ": " << sig.intf << "::" << sig.signal << std::endl;
#endif

                switch (sig.type) {
                    case ST_PROPERTIESCHANGED:
                    {
                        std::string source;
                        std::vector<std::pair<std::string, std::string>> entries;

                        parse_property_changed(msg, source, entries);
#ifndef NDEBUG
                        std::cerr << "Source: " << source << std::endl;
#endif
                        if (source != "org.mpris.MediaPlayer2.Player")
                            break;

                        std::lock_guard<std::mutex> lk(spotify.mutex);
                        bool artists_cleared = false;
                        for (auto& kv : entries) {
#ifndef NDEBUG
                            std::cerr << kv.first << " = " << kv.second << std::endl;
#endif
                            if (kv.first.find("artist") != std::string::npos) {
                                if (!artists_cleared) {
                                    artists_cleared = true;
                                    spotify.artists.clear();
                                }
                                spotify.artists.push_back(kv.second);
                            }
                            if (kv.first.find("title") != std::string::npos)
                                spotify.title = kv.second;
                            if (kv.first.find("artUrl") != std::string::npos)
                                spotify.artUrl = kv.second;
                        }
                        if (spotify.artists.size() || !spotify.title.empty())
                            spotify.valid = true;
                    }
                    break;
                    case ST_NAMEOWNERCHANGED:
                    {
                        DBusMessageIter iter;
                        dbus_message_iter_init (msg, &iter);
                        std::vector<std::string> str;
                        const char *value = nullptr;

                        while (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
                            dbus_message_iter_get_basic (&iter, &value);
                            str.push_back(value);
                            dbus_message_iter_next (&iter);
                        }

                        // did spotify quit?
                        if (str.size() == 3
                            && str[0] == "org.mpris.MediaPlayer2.spotify"
                            && str[2].empty()
                        )
                        {
                            spotify.valid = false;
                        }
                    }
                    break;
                    default:
                    break;
                }
                if (dbus_error_is_set(&error)) {
                    std::cerr << error.message << std::endl;
                    dbus_error_free(&error);
                }
            }
        }

        // free the message
        dbus_message_unref(msg);
    }
}

   dbus_manager dbus_mgr;
}