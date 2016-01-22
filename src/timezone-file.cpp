/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <datetime/timezone-file.h>

#include <gio/gio.h>

#include <cerrno>
#include <cstdlib>

namespace unity {
namespace indicator {
namespace datetime {

/***
****
***/

class FileTimezone::Impl
{
public:

    Impl(FileTimezone& owner, const std::string& filename):
        m_owner(owner)
    {
        set_filename(filename);
    }

    ~Impl()
    {
        clear();
    }

private:

    void clear()
    {
        if (m_monitor_handler_id)
            g_signal_handler_disconnect(m_monitor, m_monitor_handler_id);

        g_clear_object (&m_monitor);

        m_filename.clear();
    }

    void set_filename(const std::string& filename)
    {
        clear();

        auto tmp = realpath(filename.c_str(), nullptr);
        if(tmp != nullptr)
        {
            m_filename = tmp;
            free(tmp);
        }
        else
        {
            g_warning("Unable to resolve path '%s': %s", filename.c_str(), g_strerror(errno));
            m_filename = filename; // better than nothing?
        }

        auto file = g_file_new_for_path(m_filename.c_str());
        GError * err = nullptr;
        m_monitor = g_file_monitor_file(file, G_FILE_MONITOR_NONE, nullptr, &err);
        g_object_unref(file);
        if (err)
        {
            g_warning("%s Unable to monitor timezone file '%s': %s", G_STRLOC, TIMEZONE_FILE, err->message);
            g_error_free(err);
        }
        else
        {
            m_monitor_handler_id = g_signal_connect_swapped(m_monitor, "changed", G_CALLBACK(on_file_changed), this);
            g_debug("%s Monitoring timezone file '%s'", G_STRLOC, m_filename.c_str());
        }

        reload();
    }

    static void on_file_changed(gpointer gself)
    {
        static_cast<Impl*>(gself)->reload();
    }

    void reload()
    {
        const auto new_timezone = get_timezone_from_file(m_filename);

        if (!new_timezone.empty())
            m_owner.timezone.set(new_timezone);
    }

    /***
    ****
    ***/

    std::string get_timezone_from_file(const std::string& filename)
    {
        GError * error;
        GIOChannel * io_channel;
        std::string ret;

        // read through filename line-by-line until we fine a nonempty non-comment line
        error = nullptr;
        io_channel = g_io_channel_new_file(filename.c_str(), "r", &error);
        if (error == nullptr)
        {
            auto line = g_string_new(nullptr);

            while(ret.empty())
            {
                const auto io_status = g_io_channel_read_line_string(io_channel, line, nullptr, &error);
                if ((io_status == G_IO_STATUS_EOF) || (io_status == G_IO_STATUS_ERROR))
                    break;
                if (error != nullptr)
                    break;

                g_strstrip(line->str);

                if (!line->len) // skip empty lines
                    continue;

                if (*line->str=='#') // skip comments
                    continue;

                ret = line->str;
            }

            g_string_free(line, true);
        }

        if (io_channel != nullptr)
        {
            g_io_channel_shutdown(io_channel, false, nullptr);
            g_io_channel_unref(io_channel);
        }

        if (error != nullptr)
        {
            g_warning("%s Unable to read timezone file '%s': %s", G_STRLOC, filename.c_str(), error->message);
            g_error_free(error);
        }

        return ret;
    }

    /***
    ****
    ***/

    FileTimezone & m_owner;
    std::string m_filename;
    GFileMonitor * m_monitor = nullptr;
    unsigned long m_monitor_handler_id = 0;
};

/***
****
***/

FileTimezone::FileTimezone(const std::string& filename):
    impl(new Impl{*this, filename})
{
}

FileTimezone::~FileTimezone()
{
}

/***
****
***/

} // namespace datetime
} // namespace indicator
} // namespace unity