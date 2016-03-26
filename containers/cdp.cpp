//
// open horizon -- undefined_darkness@outlook.com
//

#include "cdp.h"
#include <assert.h>

//------------------------------------------------------------

bool cdp_file::open(const char *name)
{
    close();
    if (!name)
        return false;

    if (!nya_resources::check_extension(name, ".CDP"))
    {
        nya_resources::log()<<"invalid extension, should be .CDP\n";
        return false;
    }

    std::string tbl_name(name);
    tbl_name.resize(tbl_name.size() - 3);
    tbl_name.append("TBL");

    auto tbl_data = nya_resources::get_resources_provider().access(tbl_name.c_str());
    if (!tbl_data)
    {
        nya_resources::log()<<"unable to open corresponding .TBL file\n";
        return false;
    }

    std::vector<uint32_t> tbls(tbl_data->get_size() / sizeof(uint32_t));
    tbl_data->read_all(tbls.data());
    tbl_data->release();

    m_data = nya_resources::get_resources_provider().access(name);
    if (!m_data)
    {
        nya_resources::log()<<"unable to open .CDP file\n";
        return false;
    }

    size_t offset = 0;
    m_entries.resize(tbls.size() / 2);
    for (auto &e: m_entries)
    {
        e.offset = tbls[offset++] * 2048;
        e.size = tbls[offset++];

        assert(e.offset + e.size <= m_data->get_size());
    }

    return true;
}

//------------------------------------------------------------

void cdp_file::close()
{
    if (m_data)
        m_data->release();
    m_entries.clear();
    m_data = 0;
}

//------------------------------------------------------------

uint32_t cdp_file::get_file_size(int idx) const
{
    if (idx < 0 || idx >= get_files_count())
        return 0;

    return m_entries[idx].size;
}

//------------------------------------------------------------

bool cdp_file::read_file_data(int idx, void *data) const
{
    return read_file_data(idx, data, get_file_size(idx));
}

//------------------------------------------------------------

bool cdp_file::read_file_data(int idx, void *data, uint32_t size, uint32_t offset) const
{
    if (idx < 0 || idx >= get_files_count() || !m_data || !data)
        return false;

    const auto &e = m_entries[idx];
    if (offset + size > e.size)
        return false;

    return m_data->read_chunk(data, size, e.offset + offset);
}

//------------------------------------------------------------
