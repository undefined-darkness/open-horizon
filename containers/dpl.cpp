//
// open horizon -- undefined_darkness@outlook.com
//

#include "dpl.h"
#include "decrypt.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "resources/resources.h"
#include <assert.h>
#include <stdint.h>
#include "util/util.h"
#include "util/zip.h"

//------------------------------------------------------------

bool dpl_file::open(const char *name)
{
    close();

    m_data = nya_resources::get_resources_provider().access(name);
    if (!m_data)
    {
        nya_resources::log()<<"unable to open dpl file\n";
        return false;
    }

    struct dpl_header
    {
        char sign[4];
        uint32_t byte_order_20101010;
        uint32_t timestamp;
        uint32_t infos_count;
        uint32_t infos_size;
    } header;

    m_data->read_chunk(&header, sizeof(header));
    if (memcmp(header.sign, "DPL\1", sizeof(header.sign)) != 0)
       return false;

    m_byte_order = header.byte_order_20101010 != 20101010;
    if (m_byte_order)
    {
        header.byte_order_20101010 = swap_bytes(header.byte_order_20101010);
        header.timestamp = swap_bytes(header.timestamp);
        header.infos_count = swap_bytes(header.infos_count);
        header.infos_size = swap_bytes(header.infos_size);
    }

    assert(header.byte_order_20101010 == 20101010);

    m_archieved = header.timestamp != 2011082201; //ToDo?

    nya_memory::tmp_buffer_scoped buf(header.infos_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), sizeof(header));
    nya_memory::memory_reader r(buf.get_data(), buf.get_size());

    m_infos.resize(header.infos_count);

    struct entry
    {
        uint64_t offset;
        uint32_t size;
        uint32_t idx;
        uint32_t unknown;
        uint8_t key;
    };

    struct unknown_struct
    {
        uint32_t idx;
        uint32_t unknown;
        uint32_t unknown_pot;
    };

    for (uint32_t i = 0; i < header.infos_count; ++i)
    {
        fhm_file::fhm_header h = r.read<fhm_file::fhm_header>();
        entry e = r.read<entry>();

        assert(h.check_sign());

        const bool byte_order = h.wrong_byte_order();
        assert(byte_order == m_byte_order);
        if (byte_order)
        {
            for (uint32_t j = 1; j < sizeof(h) / 4; ++j)
                ((uint32_t *)&h)[j] = swap_bytes(((uint32_t *)&h)[j]);

            for (uint32_t j = 1; j < sizeof(e) / 4; ++j)
                ((uint32_t *)&e)[j] = swap_bytes(((uint32_t *)&e)[j]);

            std::swap(((uint32_t *)&e.offset)[0], ((uint32_t *)&e.offset)[1]);
        }

        assert(h.byte_order_20101010 == 20101010);
        assume(h.timestamp == header.timestamp);
        //assert(e.idx == (m_archieved ? i+10000 : i));
        assert(e.offset+e.size <= (uint64_t)m_data->get_size());
        //assert(e.arch_unknown_16 == (m_archieved ? 16 : 0));

        for (uint32_t j = 0; j < h.unknown_struct_count; ++j)
        {
            unknown_struct s = r.read<unknown_struct>();
            if (byte_order)
            {
                for (uint32_t j = 0; j < sizeof(s) / 4; ++j)
                    ((uint32_t *)&s)[j] = swap_bytes(((uint32_t *)&s)[j]);
            }

            assert(s.idx <= h.unknown_struct_count);
        }

        m_infos[i].header = h;
        m_infos[i].offset = e.offset;
        m_infos[i].size = e.size;
        m_infos[i].unpacked_size = m_archieved ? h.size + sizeof(fhm_file::fhm_header) : e.size;
        m_infos[i].key = e.key;
    }

    return true;
}

//------------------------------------------------------------

void dpl_file::close()
{
    if (m_data)
        m_data->release();

    m_data = 0;
    m_infos.clear();
}

//------------------------------------------------------------

uint32_t dpl_file::get_file_size(int idx) const
{
    if (idx < 0 || idx >= (int)m_infos.size())
        return 0;

    return m_infos[idx].unpacked_size;
}

//------------------------------------------------------------

bool dpl_file::read_file_data(int idx, void *data) const
{
    if (!data || idx < 0 || idx >= (int)m_infos.size())
        return false;

    const auto &e = m_infos[idx];

    if (!m_archieved)
        return m_data->read_chunk(data, e.size, (size_t)e.offset);

    memcpy(data, &e.header, sizeof(e.header));
    data = (char *)data + sizeof(e.header);

    nya_memory::tmp_buffer_scoped buf(e.size);
    if (!m_data->read_chunk(buf.get_data(), e.size, (size_t)e.offset))
        return false;

    struct block_header
    {
        uint16_t unknown_323; //323 or 143h
        uint16_t idx;
        uint32_t unknown;
        uint32_t unpacked_size;
        uint32_t packed_size;
    } header;

    char *buf_out = (char *)data;

    nya_memory::memory_reader r(buf.get_data(), buf.get_size());
    uint16_t curr_idx = 0;
    while (r.check_remained(sizeof(header)))
    {
        header = r.read<block_header>();
        assume(header.unknown_323 == 323);
        assert(header.idx == curr_idx++);

        if (m_byte_order)
        {
            for (uint32_t j = 1; j < sizeof(header) / 4; ++j)
                ((uint32_t *)&header)[j] = swap_bytes(((uint32_t *)&header)[j]);
        }

        uint8_t *buf_from = (uint8_t *)r.get_data();
        if (!r.check_remained(header.packed_size))
            return false;

        decrypt(buf_from, header.packed_size, e.key);
        const bool success=unzip(buf_from, header.packed_size, buf_out, header.unpacked_size);
        if (!success)
        {
            if (header.packed_size != header.unpacked_size)
                return false;

            memcpy(buf_out, buf_from, header.packed_size);
        }

        buf_out += header.unpacked_size;
        r.skip(header.packed_size);
    }

    return true;
}

//------------------------------------------------------------
