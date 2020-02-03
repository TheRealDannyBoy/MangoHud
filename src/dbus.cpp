#include <stdexcept>
#include <cstdio>
#include <dbus/dbus.h>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>
#include "dbus_info.h"

struct metadata spotify;

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
        std::cerr << "Not an array " << current_type << std::endl;
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

            // dict entry value
            dbus_message_iter_next (&iter);

            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_VARIANT)
                dbus_message_iter_recurse (&iter, &iter);

            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_ARRAY) {
                dbus_message_iter_recurse (&iter, &iter);
                if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
                    dbus_message_iter_get_basic (&iter, &val_value);
                    std::string key = val_key;
                    if (key.find("artist") != std::string::npos)
                        spotify.artists.insert(spotify.artists.begin(), val_value);
                }
            }

            // printf("%c}\n", dbus_message_iter_get_arg_type (&iter));
            if (dbus_message_iter_get_arg_type (&iter) == DBUS_TYPE_STRING) {
                dbus_message_iter_get_basic (&iter, &val_value);
                std::string key = val_key;
                if (key.find("title") != std::string::npos)
                    spotify.title = val_value;

                if (key.find("artUrl") != std::string::npos)
                    spotify.artUrl = val_value;
                entries.push_back({val_key, val_value});
            }
        }
        dbus_message_iter_next (&subiter);
    }

}

void spotifyMetadata()
{
    spotify.artists.clear();
    ::dbus_threads_init_default();

    DBusError error;
    ::dbus_error_init(&error);
    DBusConnection * dbus_conn = nullptr;
    DBusMessage * dbus_reply = nullptr;
    DBusMessage * dbus_msg = nullptr;
    if ( nullptr == (dbus_conn = ::dbus_bus_get(DBUS_BUS_SESSION , &error)) ) {
       throw std::runtime_error(error.message);
    }
/*  if (nullptr == (dbus_msg = ::dbus_message_new_method_call("org.mpris.MediaPlayer2.spotify", "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Metadata"))) {
       throw std::runtime_error("unable to allocate memory for dbus message");
    }*/
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
    if (nullptr == (dbus_reply = ::dbus_connection_send_with_reply_and_block(dbus_conn, dbus_msg, DBUS_TIMEOUT_USE_DEFAULT, &error))) {
        ::dbus_message_unref(dbus_msg);
        throw std::runtime_error(error.message);
    } else {

        std::vector<std::pair<std::string, std::string>> entries;
        get_dict_string_array(dbus_reply, entries);

        // std::cout << "String entries:\n";
        for(auto& p : entries) {
            // std::cout << p.first << std::endl;
        }

    }

    ::dbus_message_unref(dbus_msg);
    ::dbus_message_unref(dbus_reply);
}