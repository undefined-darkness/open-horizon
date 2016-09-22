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

namespace
{

struct fhm_ac6_header
{
    char sign[4];
    uint32_t byte_order_0x01010010;
    uint32_t unknown_zero[2];
    uint32_t count;

    bool check_sign() const { return memcmp(sign, "FHM ", 4) == 0; }
    bool wrong_byte_order() const { return byte_order_0x01010010 != 0x01010010 && byte_order_0x01010010 != 0x20101010; }
};

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

    fhm_ac6_header ac6_header;
    if (!m_data->read_chunk(&ac6_header, sizeof(ac6_header))) //ac6 header is smaller than fhm header
    {
        nya_resources::log()<<"invalid fhm file\n";
        close();
        return false;
    }

    if (ac6_header.check_sign())
    {
        m_byte_order = true;
        ac6_header.byte_order_0x01010010 = swap_bytes(ac6_header.byte_order_0x01010010);
        ac6_header.count = swap_bytes(ac6_header.count);

        assert(!ac6_header.wrong_byte_order());
        assume(ac6_header.unknown_zero[0] == 0 && ac6_header.unknown_zero[1] == 0);

        read_ac6_chunks_info(0, ac6_header.count, m_root);
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

    read_chunks_info(sizeof(header), m_root);
    return true;
}

//------------------------------------------------------------

bool fhm_file::read_chunks_info(size_t base_offset, folder &f)
{
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
            f.folders.push_back({});
            read_chunks_info(offset + base_offset, f.folders.back());
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

        f.files.push_back((int)m_chunks.size());
        m_chunks.push_back(c);

        //assert((chunk_info.unknown1 == 1 && chunk_info.unknown2 == 2) || (chunk_info.unknown1 == 0 && chunk_info.unknown2 == 0));
        assert(chunk_info.unknown_pot == 16 || chunk_info.unknown_pot == 128 || chunk_info.unknown_pot == 4096);
    }

    return true;
}

//------------------------------------------------------------

bool fhm_file::read_ac6_chunks_info(uint32_t base_offset, int count, folder &f)
{
    if (!count)
        return true;

    assert(count > 0);

    std::vector<uint32_t> offsets(count * 2);
    if (!m_data->read_chunk(offsets.data(), count * 2 * sizeof(uint32_t), base_offset + sizeof(fhm_ac6_header)))
    {
        nya_resources::log()<<"invalid ac6 fhm file\n";
        close();
        return false;
    }

    std::vector<chunk> chunks;

    chunks.resize(count);

    size_t offset = 0;

    for (auto &c: chunks)
        c.offset = swap_bytes(offsets[offset++]) + base_offset;
    for (auto &c: chunks)
        c.size = swap_bytes(offsets[offset++]);

    for (auto &c: chunks)
    {
        if (c.size == 0)
        {
            c.offset = 0;
            continue;
        }

        assert(c.offset + c.size <= m_data->get_size());

        c.type = 0;
        if (c.size > 4)
            m_data->read_chunk(&c.type, 4, c.offset);

        if (memcmp(&c.type, "FHM", 3) == 0)
        {
            fhm_ac6_header header;
            if (!m_data->read_chunk(&header, sizeof(header), c.offset))
                continue;

            f.folders.push_back({});
            read_ac6_chunks_info(c.offset, swap_bytes(header.count), f.folders.back());
        }
        else
        {
            f.files.push_back((int)m_chunks.size());
            m_chunks.push_back(c);
        }
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
    m_root = folder();
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

template<typename t> void debug_print(const fhm_file &fhm, t &folder, int nesting)
{
    for (auto &f: folder.folders)
    {
        for (int i = 0; i < nesting; ++i)
            printf("=");
        printf("<folder>\n");
        debug_print(fhm, f, nesting + 1);
    }

    for (auto &f: folder.files)
    {
        for (int i = 0; i < nesting; ++i)
            printf("-");
        auto type = fhm.get_chunk_type(f);
        printf("%2d %.4s %.2fMb\n", f, (char *)&type, fhm.get_chunk_size(f) / (1024.0f * 1024));
    }

    printf("\n");
}

void fhm_file::debug_print() const
{
    printf("fhm: \n");
    ::debug_print(*this, m_root, 0);
}

//------------------------------------------------------------
