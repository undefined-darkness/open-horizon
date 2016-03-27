//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm.h"
#include "util/util.h"

//------------------------------------------------------------

bool fhm_file::open(const char *name)
{
    return open(nya_resources::get_resources_provider().access(name));
}

//------------------------------------------------------------

bool fhm_file::open(nya_resources::resource_data *data)
{
    close();

    if (!data)
    {
        nya_resources::log()<<"unable to open fhm file\n";
        return false;
    }

    m_data = data;

    struct fhm_old_header //AC6
    {
        char sign[4];
        uint32_t byte_order_0x01010010;
        uint32_t unknown_zero[2];
        uint32_t count;

        bool check_sign() const { return memcmp(sign, "FHM ", 4) == 0; }
        bool wrong_byte_order() const { return byte_order_0x01010010 != 0x01010010; }
    };

    fhm_old_header old_header;
    if (!m_data->read_chunk(&old_header, sizeof(old_header))) //old header is smaller than fhm header
    {
        nya_resources::log()<<"invalid fhm file\n";
        close();
        return false;
    }

    if (old_header.check_sign())
    {
        m_byte_order = true;
        old_header.byte_order_0x01010010 = swap_bytes(old_header.byte_order_0x01010010);
        old_header.count = swap_bytes(old_header.count);

        assert(!old_header.wrong_byte_order());
        assume(old_header.unknown_zero[0] == 0 && old_header.unknown_zero[1] == 0);

        std::vector<uint32_t> offsets(old_header.count * 2);
        if (!m_data->read_chunk(offsets.data(), old_header.count * 2 * sizeof(uint32_t), sizeof(old_header)))
        {
            nya_resources::log()<<"invalid ac6 fhm file\n";
            close();
            return false;
        }

        m_chunks.resize(old_header.count);

        size_t offset = 0;
        for (auto &c: m_chunks)
            c.offset = swap_bytes(offsets[offset++]);
        for (auto &c: m_chunks)
            c.size = swap_bytes(offsets[offset++]);

        for (auto &c: m_chunks)
        {
            assert(c.offset + c.size <= m_data->get_size());

            if (c.size >= 4)
                m_data->read_chunk(&c.type, 4, c.offset);
        }

        return true;
    }

    fhm_header header;
    if (!m_data->read_chunk(&header, sizeof(header), 0) || !header.check_sign())
    {
        nya_resources::log()<<"invalid fhm file\n";
        close();
        return false;
    }

    assert(!header.wrong_byte_order());
    assert(header.size + sizeof(header) <= m_data->get_size());
    assume(header.size + sizeof(header) == m_data->get_size());

    int group = 0;
    read_chunks_info(sizeof(header), 0, group);
    return true;
}

//------------------------------------------------------------

bool fhm_file::read_chunks_info(size_t base_offset, int nesting, int &group)
{
    const int g = group++;
    unsigned int chunks_count = 0;
    m_data->read_chunk(&chunks_count, 4, base_offset);

    for (int i = 0; i < chunks_count; ++i)
    {
        unsigned int nested = 0, offset = 0;
        size_t off = base_offset + 4 + i * 8;
        m_data->read_chunk(&nested, 4, off); off+=4;
        m_data->read_chunk(&offset, 4, off); off+=4;

        if (nested == 1)
        {
            read_chunks_info(offset + base_offset, nesting + 1, group);
            continue;
        }

        assert(nested == 0);

        struct
        {
            unsigned short unknown1;
            unsigned short unknown2;
            unsigned int unknown_pot;
            unsigned int offset; //+header size(48)
            unsigned int size;
        } chunk_info;

        const bool read_ok = m_data->read_chunk(&chunk_info, sizeof(chunk_info), base_offset + offset);
        assert(read_ok);

        chunk c;
        c.offset = chunk_info.offset + sizeof(fhm_header);
        c.size = chunk_info.size;
        c.type = 0;
        if (c.size >= 4)
            m_data->read_chunk(&c.type, 4, c.offset);

        //for(int j=0;j<nesting;++j) printf("-/"); printf("chunk %d %d %d %.4s %d\n", (uint32_t)m_chunks.size(), nesting, g, (char *)&c.type, c.type);

        m_chunks.push_back(c);

        //assert((chunk_info.unknown1 == 1 && chunk_info.unknown2 == 2) || (chunk_info.unknown1 == 0 && chunk_info.unknown2 == 0));
        assert(chunk_info.unknown_pot == 16 || chunk_info.unknown_pot == 128 || chunk_info.unknown_pot == 4096);
    }

    return true;
}

//------------------------------------------------------------

void fhm_file::close()
{
    if (m_data)
        m_data->release();

    m_data = 0;
    m_chunks.clear();
    m_byte_order = false;
}

//------------------------------------------------------------

uint32_t fhm_file::get_chunk_offset(int idx) const
{
    if (idx < 0 || idx >= int(m_chunks.size()))
        return 0;

    return m_chunks[idx].offset;
}

//------------------------------------------------------------

uint32_t fhm_file::get_chunk_size(int idx) const
{
    if (idx < 0 || idx >= int(m_chunks.size()))
        return 0;

    return m_chunks[idx].size;
}

//------------------------------------------------------------

bool fhm_file::read_chunk_data(int idx, void *data) const
{
    if (idx < 0 || idx >= int(m_chunks.size()))
        return 0;

    return m_data->read_chunk(data, m_chunks[idx].size, m_chunks[idx].offset);
}

//------------------------------------------------------------

uint32_t fhm_file::get_chunk_type(int idx) const
{
    if (idx < 0 || idx >= int(m_chunks.size()))
        return 0;

    return m_chunks[idx].type;
}

//------------------------------------------------------------
