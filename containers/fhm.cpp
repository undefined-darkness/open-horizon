//
// open horizon -- undefined_darkness@outlook.com
//

#include "fhm.h"
#include <assert.h>

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

    fhm_header header;
    m_data->read_chunk(&header, sizeof(header), 0);
    if (!header.check_sign())
    {
        nya_resources::log()<<"invalid fhm file\n";
        close();
        return false;
    }

    assert(!header.wrong_byte_order()); //ToDo
    assert(header.size + sizeof(header) == m_data->get_size()); //assumption

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

        //for(int j=0;j<nested;++j) printf("\t"); printf("chunk %d %d %c%c%c%c %d\n", nested, g, ((char *)&c.type)[0], ((char *)&c.type)[1], ((char *)&c.type)[2], ((char *)&c.type)[3], c.type);

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
