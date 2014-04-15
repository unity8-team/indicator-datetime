/*
 * Copyright 2014 Canonical Ltd.
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

#include <datetime/planner-range.h>

namespace unity {
namespace indicator {
namespace datetime {

/***
****
***/

SimpleRangePlanner::SimpleRangePlanner(const std::shared_ptr<Engine>& engine,
                                   const std::shared_ptr<Timezone>& timezone):
    m_engine(engine),
    m_timezone(timezone),
    m_range(std::pair<DateTime,DateTime>(DateTime::NowLocal(), DateTime::NowLocal()))
{
    range().changed().connect([this](const std::pair<DateTime,DateTime>&){
        g_debug("rebuilding because the date range changed");
        rebuild_soon();
    });
}

SimpleRangePlanner::~SimpleRangePlanner()
{
    if (m_rebuild_tag)
        g_source_remove(m_rebuild_tag);
}

/***
****
***/

void SimpleRangePlanner::rebuild_now()
{
    const auto& r = range().get();
    m_engine->set_range(r.first, r.second);
}

void SimpleRangePlanner::rebuild_soon()
{
    static const int ARBITRARY_BATCH_MSEC = 200;

    if (m_rebuild_tag == 0)
        m_rebuild_tag = g_timeout_add(ARBITRARY_BATCH_MSEC, rebuild_now_static, this);
}

gboolean SimpleRangePlanner::rebuild_now_static(gpointer gself)
{
    auto self = static_cast<SimpleRangePlanner*>(gself);
    self->m_rebuild_tag = 0;
    self->rebuild_now();
    return G_SOURCE_REMOVE;
}

/***
****
***/

core::Property<std::vector<Appointment>>& SimpleRangePlanner::appointments()
{
    return m_engine->appointments();
}

core::Property<std::pair<DateTime,DateTime>>& SimpleRangePlanner::range()
{
    return m_range;
}

/***
****
***/

} // namespace datetime
} // namespace indicator
} // namespace unity
