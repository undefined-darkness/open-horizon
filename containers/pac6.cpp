//
// open horizon -- undefined_darkness@outlook.com
//

#include "pac6.h"
#include "util/util.h"

//------------------------------------------------------------

bool pac6_file::open(const char *name)
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
    assert(tbl_name.size() > strlen("00.PAC"));
    tbl_name.resize(tbl_name.size() - strlen("00.PAC"));
    tbl_name.append(".TBL");

    auto tbl_data = nya_resources::get_resources_provider().access(tbl_name.c_str());
    if (!tbl_data)
    {
        nya_resources::log()<<"unable to open corresponding .TBL file\n";
        return false;
    }

    std::vector<uint32_t> tbls(tbl_data->get_size() / sizeof(uint32_t));
    tbl_data->read_all(tbls.data());
    tbl_data->release();

    for (auto &t: tbls)
        t = swap_bytes(t);

    size_t offset = 0;

    const auto count = tbls[offset++];
    const auto tomes_count = tbls[offset++];

    assert(tomes_count < 10);

    m_data.resize(tomes_count);
    for (int i = 0; i < tomes_count; ++i)
    {
        std::string name_str(name);
        assert(name_str.length() > strlen("0.PAC"));
        name_str[name_str.length()-strlen("0.PAC")] += i;

        m_data[i] = nya_resources::get_resources_provider().access(name_str.c_str());
        if (!m_data[i])
        {
            for (int j = 0; j < i; ++j)
                m_data[j]->release();

            nya_resources::log()<<"unable to open "<<name_str<<" file\n";
            return false;
        }
    }

    m_entries.resize(count);
    for (auto &e: m_entries)
    {
        union info { uint8_t c[4]; uint32_t u; } i;
        i.u = tbls[offset++];

        assume(i.c[0] == 0 && i.c[1] == 0);
        assume(i.c[2] == 1 || i.c[2] == 2);

        e.compressed = i.c[2] == 1;
        e.tome = i.c[3];
        e.offset = tbls[offset++];
        e.size = tbls[offset++];
        e.unpacked_size = tbls[offset++];

        assume(e.compressed || e.size == e.unpacked_size);

        assert(e.tome < m_data.size());
        assert(e.offset + e.size <= m_data[e.tome]->get_size());
    }

    return true;
}

//------------------------------------------------------------

void pac6_file::close()
{
    m_entries.clear();
    for (auto &d: m_data)
        d->release();
    m_data.clear();
}

//------------------------------------------------------------

uint32_t pac6_file::get_file_size(int idx) const
{
    if (idx < 0 || idx >= get_files_count())
        return 0;

    return m_entries[idx].unpacked_size;
}

//------------------------------------------------------------
