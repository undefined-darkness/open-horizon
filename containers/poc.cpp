//
// open horizon -- undefined_darkness@outlook.com
//

#include "poc.h"
#include <string.h>
#include <assert.h>

//------------------------------------------------------------

bool poc_file::open(const char *name)
{
    return open(nya_resources::get_resources_provider().access(name));
}

//------------------------------------------------------------

bool poc_file::open(nya_resources::resource_data *data)
{
    close();
    m_data = data;
    if (!m_data)
        return false;

    uint32_t count = 0;
    if (!m_data->read_chunk(&count, sizeof(uint32_t))
        || (count + 1) > m_data->get_size() / sizeof(uint32_t)) //check overflow
    {
        close();
        return false;
    }

    std::vector<uint32_t> offsets(count);
    if (!m_data->read_chunk(offsets.data(), count * sizeof(uint32_t), sizeof(uint32_t)))
    {
        close();
        return false;
    }

    return init((uint32_t *)m_raw_data + 1, count, (uint32_t)m_data->get_size());
}

//------------------------------------------------------------

bool poc_file::open(const void *data, size_t size)
{
    close();
    m_raw_data = (char *)data;
    if (!m_raw_data || size < 4)
        return false;

    uint32_t count = *(uint32_t *)m_raw_data;
    if ((count + 1) * sizeof(uint32_t) > size
        || (count + 1) > size / sizeof(uint32_t)) //check overflow
    {
        m_raw_data = 0;
        return false;
    }

    return init((uint32_t *)m_raw_data + 1, count, (uint32_t)size);
}

//------------------------------------------------------------

bool poc_file::init(const uint32_t *offsets, uint32_t count, uint32_t size)
{
    //check if it's a valid container

    if (!count || count > 1024)
    {
        close();
        return false;
    }

    for (uint32_t i = 0; i < count; ++i)
    {
        if (offsets[i] >= size)
        {
            close();
            return false;
        }
    }

    const uint32_t min_offset = (count + 1) * sizeof(uint32_t);
    uint32_t last_good_offset = 0;

    for (uint32_t i = 0; i < count; ++i)
    {
        if (!offsets[i])
            continue;

        if (offsets[i] < min_offset || offsets[i] < last_good_offset)
        {
            close();
            return false;
        }

        last_good_offset = offsets[i];
    }

    if (last_good_offset == 0)
        return false;

    //read entries

    m_entries.resize(count);
    for (size_t i = 0; i < m_entries.size(); ++i)
    {
        auto &e = m_entries[i];
        e.offset = offsets[i];
        e.size = 0;
        if (!e.offset)
            continue;

        for (size_t j = i + 1; j < m_entries.size(); ++j)
        {
            if (!offsets[j])
                continue;

            e.size = offsets[j] - e.offset;
            break;
        }
    }

    if (m_entries.back().offset > 0)
        m_entries.back().size = size - m_entries.back().offset;

    int idx = 0;
    for (auto &e: m_entries)
    {
        e.type = 0;
        if (e.size >= sizeof(uint32_t))
            read_chunk_data(idx, &e.type, sizeof(uint32_t));
        ++idx;
    }

    return true;
}

//------------------------------------------------------------

void poc_file::close()
{
    if (m_data)
        m_data->release();
    m_entries.clear();
    m_data = 0;
    m_raw_data = 0;
}

//------------------------------------------------------------

uint32_t poc_file::get_chunk_type(int idx) const
{
    if (idx < 0 || idx >= get_chunks_count())
        return 0;

    return m_entries[idx].type;
}

//------------------------------------------------------------

uint32_t poc_file::get_chunk_size(int idx) const
{
    if (idx < 0 || idx >= get_chunks_count())
        return 0;

    return m_entries[idx].size;
}

//------------------------------------------------------------

uint32_t poc_file::get_chunk_offset(int idx) const
{
    if (idx < 0 || idx >= get_chunks_count())
        return 0;

    return m_entries[idx].size;
}

//------------------------------------------------------------

bool poc_file::read_chunk_data(int idx, void *data) const
{
    return read_chunk_data(idx, data, get_chunk_size(idx));
}

//------------------------------------------------------------

bool poc_file::read_chunk_data(int idx, void *data, uint32_t size, uint32_t offset) const
{
    if (idx < 0 || idx >= get_chunks_count() || !data)
        return false;

    const auto &e = m_entries[idx];
    if (offset + size > e.size)
        return false;

    if (m_data)
        return m_data->read_chunk(data, size, e.offset + offset);

    return memcpy(data, m_raw_data + e.offset + offset, size) != 0;
}

//------------------------------------------------------------
