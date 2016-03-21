//
// open horizon -- undefined_darkness@outlook.com
//

#include "pac5.h"
#include "memory/tmp_buffer.h"
#include <assert.h>

//------------------------------------------------------------

bool pac5_file::open(const char *name)
{
    close();
    if (!name)
        return false;

    if (!nya_resources::check_extension(name, ".PAC"))
    {
        nya_resources::log()<<"invalid extension, should be .PAC\n";
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
        nya_resources::log()<<"unable to open .PAC file\n";
        return false;
    }

    size_t offset = 0;
    const auto count = tbls[offset++];
    const auto unknown_zero = tbls[offset++];
    assert(unknown_zero == 0);

    m_entries.resize(count);
    for (auto &e: m_entries)
    {
        e.offset = tbls[offset++];
        e.size = tbls[offset++];

        assert(e.offset + e.size <= m_data->get_size());
    }

    for (auto &e: m_entries)
        e.unpacked_size = tbls[offset++];

    return true;
}

//------------------------------------------------------------

void pac5_file::close()
{
    if (m_data)
        m_data->release();
    m_entries.clear();
}

//------------------------------------------------------------

uint32_t pac5_file::get_file_size(int idx) const
{
    if (idx < 0 || idx >= get_files_count())
        return 0;

    return m_entries[idx].unpacked_size;
}

//------------------------------------------------------------

static bool uncompress_ulz2(const uint8_t *in_data, uint32_t in_size, uint8_t *out_data)
{
    struct ulz_header
    {
        char sign[4];
        uint32_t size;
        uint32_t pos;
        uint32_t count_pos;
    };

    ulz_header h = *(const ulz_header *)in_data;
    if (memcmp(h.sign, "Ulz\x1a", 4) != 0)
        return false;

    const uint32_t type = h.size >> 24;
    if (type != 2)
        return false;

    const uint16_t c1 = (uint16_t)((h.pos & 0xFF000000) >> 24);
    const uint16_t c2 = (uint16_t)((1 << (uint32_t)c1) + 0xFFFF);
    h.size &= 0x00FFFFFF;
    h.pos &= 0x00FFFFFF;

    const int *flags = (int *)(in_data + sizeof(ulz_header));
    int flag = *flags, flag_upd = 0;

    const uint16_t *counts = (uint16_t *)(in_data + h.count_pos);

    in_data += h.pos;

    while (h.size)
    {
        if (!flag_upd--)
        {
            flag = *flags++;
            flag_upd = 31;
        }

        const bool is_comp = flag >= 0;
        flag <<= 1;

        if (!is_comp)
        {
            *out_data++ = *in_data++, --h.size;
            continue;
        }

        const uint16_t c = *counts++;
        const uint32_t t_pos = (c & c2) + 1;
        const uint32_t count = (c >> c1) + 3;
        if (count > h.size)
            return false;

        auto *tmp = out_data - t_pos;
        for (uint32_t j = 0; j < count; j++)
            *out_data++ = tmp[j];

        h.size -= count;
    }

    return true;
}

//------------------------------------------------------------

bool pac5_file::read_file_data(int idx, void *data) const
{
    if (idx < 0 || idx >= get_files_count() || !data)
        return false;

    auto &e = m_entries[idx];
    nya_memory::tmp_buffer_scoped buf(e.size);
    m_data->read_chunk(buf.get_data(), e.size, e.offset);
    return uncompress_ulz2((uint8_t *)buf.get_data(), e.size, (uint8_t *)data);
}

//------------------------------------------------------------
