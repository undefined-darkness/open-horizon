//
// open horizon -- undefined_darkness@outlook.com
//

#include "dpl.h"
#include "dpl_keys.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "resources/resources.h"
#include <assert.h>
#include <zlib.h>
#include <stdint.h>
#include "util/util.h"

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
        uint32_t flags;
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
        header.flags = swap_bytes(header.flags);
        header.infos_count = swap_bytes(header.infos_count);
        header.infos_size = swap_bytes(header.infos_size);
    }

    assert(header.byte_order_20101010 == 20101010);

    m_archieved = header.flags == 2011101108 || header.flags == 2011080301 || header.flags == 2013091901;
    if (!m_archieved && header.flags != 2011082201)
        return false;

    nya_memory::tmp_buffer_scoped buf(header.infos_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), sizeof(header));
    nya_memory::memory_reader r(buf.get_data(), buf.get_size());

    m_infos.resize(header.infos_count);

    struct fhm_entry
    {
        char sign[4];
        uint32_t byte_order_20101010;
        uint32_t flags;
        uint32_t unknown_struct_count;

        // next block zero if not arch
        uint32_t arch_unknown;
        uint32_t arch_unpacked_size;
        uint32_t arch_unknown3;
        uint32_t arch_unknown4;
        uint32_t arch_unknown5;
        uint32_t arch_unknown_16;
        uint32_t arch_unknown_pot;
        uint32_t arch_unknown_pot2;

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
        fhm_entry e = r.read<fhm_entry>();

        assert(memcmp(e.sign, "FHM", 3) == 0);

        const bool byte_order = e.byte_order_20101010 != 20101010;
        assert(byte_order == m_byte_order);
        if (byte_order)
        {
            for (uint32_t j = 1; j < sizeof(e) / 4; ++j)
                ((uint32_t *)&e)[j] = swap_bytes(((uint32_t *)&e)[j]);

            std::swap(((uint32_t *)&e.offset)[0], ((uint32_t *)&e.offset)[1]);
        }

        assert(e.byte_order_20101010 == 20101010);
        assert(e.flags == header.flags);
        //assert(e.idx == (m_archieved ? i+10000 : i));
        assert(e.offset+e.size <= (uint64_t)m_data->get_size());
        //assert(e.arch_unknown_16 == (m_archieved ? 16 : 0));

        for (uint32_t j = 0; j < e.unknown_struct_count; ++j)
        {
            unknown_struct s = r.read<unknown_struct>();
            if (byte_order)
            {
                for (uint32_t j = 0; j < sizeof(s) / 4; ++j)
                    ((uint32_t *)&s)[j] = swap_bytes(((uint32_t *)&s)[j]);
            }

            assert(s.idx <= e.unknown_struct_count);
        }

        m_infos[i].offset = e.offset;
        m_infos[i].size = e.size;
        m_infos[i].unpacked_size = m_archieved ? e.arch_unpacked_size : e.size;
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

static bool unzip(const void *from, size_t from_size, void *to, size_t to_size)
{
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)from_size;
    infstream.next_in = (Bytef *)from;
    infstream.avail_out = (uInt)to_size;
    infstream.next_out = (Bytef *)to;

    inflateInit2(&infstream, -MAX_WBITS);

    const int result = inflate(&infstream, Z_NO_FLUSH);
    if (result != Z_OK && result != Z_STREAM_END)
        return false;

    if (infstream.total_in != from_size || infstream.total_out != to_size)
        return false;

    inflateEnd(&infstream);
    return true;
}

//------------------------------------------------------------

bool dpl_file::read_file_data(int idx, void *data) const
{
    if (!data || idx < 0 || idx >= (int)m_infos.size())
        return false;

    const auto &e = m_infos[idx];

    if (!m_archieved)
        return m_data->read_chunk(data, e.size, (size_t)e.offset);

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
    while (r.check_remained(sizeof(header)))
    {
        header = r.read<block_header>();

        if (m_byte_order)
        {
            for (uint32_t j = 1; j < sizeof(header) / 4; ++j)
                ((uint32_t *)&header)[j] = swap_bytes(((uint32_t *)&header)[j]);
        }

        uint8_t *buf_from = (uint8_t *)r.get_data();
        if (!r.check_remained(header.packed_size))
            return false;

        auto keys = get_dpl_key(e.key);
        for (size_t i = 0; i < header.packed_size; ++i)
            buf_from[i] ^= keys[i % 8];

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

void dpl_entry::read_entries(uint32_t offset)
{
    if (!m_data)
        return;

    assert(offset < m_size);
    nya_memory::memory_reader reader((char *)m_data + offset, m_size - offset);

    uint32_t count = reader.read<uint32_t>();
    for (uint32_t i = 0; i < count; ++i)
    {
        reader.seek(4 + 8 * i);

        uint32_t has_childs = reader.read<uint32_t>();
        uint32_t entry_offset = reader.read<uint32_t>();

        assert(has_childs == 0 || has_childs == 1);

        if (has_childs > 0)
        {
            read_entries(offset + entry_offset);
            continue;
        }

        reader.seek(entry_offset);

        auto unknown = reader.read<uint16_t>();
        assert(unknown<32);
        auto unknown2 = reader.read<uint16_t>();
        assert(unknown2<16);
        auto unknown_pot = reader.read<uint32_t>();
        assert(unknown_pot == 16 || unknown_pot == 128 || unknown_pot == 4096);
        auto off = reader.read<uint32_t>();
        auto size = reader.read<uint32_t>();
        m_entries.push_back(std::make_pair(off, size));
    }
}

//------------------------------------------------------------
