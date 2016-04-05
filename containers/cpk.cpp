//
// open horizon -- undefined_darkness@outlook.com
//

//------------------------------------------------------------

//based on https://github.com/esperknight/CriPakTools

//------------------------------------------------------------

#include "cpk.h"
#include "memory/invalid_object.h"
#include "util/util.h"
#include <assert.h>

//------------------------------------------------------------

cri_utf_table::cri_utf_table(const void *data, size_t size)
{
    nya_memory::memory_reader r(data, size);
    if (!r.test("@UTF", 4))
        return;

    const auto table_size = swap_bytes(r.read<uint32_t>());
    if (table_size > r.get_remained())
        return;

    const auto utf_hoff = r.get_offset();

    struct utf_header
    {
        uint32_t rows_offset;
        uint32_t strings_offset;
        uint32_t data_offset;
        uint32_t table_name = 0;
        uint16_t num_colums = 0;
        uint16_t row_length;
        uint32_t num_rows = 0;

    } header = r.read<utf_header>();

    for (uint32_t i = 0; i < sizeof(header) / 4; ++i)
        ((uint32_t *)&header)[i] = swap_bytes(((uint32_t *)&header)[i]);
    std::swap(header.row_length, header.num_colums);

    header.rows_offset += utf_hoff;
    header.strings_offset += utf_hoff;
    header.data_offset += utf_hoff;

    std::vector<uint8_t> column_flags(header.num_colums);
    columns.resize(header.num_colums);

    for (uint32_t i = 0; i < header.num_colums; ++i)
    {
        column_flags[i] = r.read<uint8_t>();
        if (!column_flags[i])
        {
            r.skip(3);
            column_flags[i] = r.read<uint8_t>();
        }

        auto off = swap_bytes(r.read<uint32_t>());
        if (off >= table_size) //unknown special case
        {
            columns.clear();
            return;
/*
            r.seek(r.get_offset()-3);
            column_flags[i] = r.read<uint8_t>();
            off = swap_bytes(r.read<uint32_t>());
*/
        }


        off += header.strings_offset;
        assert(off < table_size);
        const char *name = (char *)data + off;
        assert(name);
        columns[i].name = name;
    }

    for (uint32_t j = 0; j < header.num_rows; ++j)
    {
        r.seek(header.rows_offset + j * header.row_length);

        for (uint32_t i = 0; i < header.num_colums; ++i)
        {
            enum
            {
                STORAGE_MASK = 0xf0,
                STORAGE_NONE = 0x00,
                STORAGE_ZERO = 0x10,
                STORAGE_CONSTANT = 0x30,
                STORAGE_PERROW = 0x50,

                TYPE_MASK = 0x0f
            };

            auto storage = column_flags[i] & STORAGE_MASK;
            if (storage == STORAGE_CONSTANT && j > 0)
            {
                columns[i].values[j] = columns[i].values[0];
                continue;
            }

            if (storage != STORAGE_PERROW && !(storage == STORAGE_CONSTANT && j == 0))
            {
                assume(storage == STORAGE_ZERO); //ToDo
                continue;
            }

            columns[i].values.resize(header.num_rows);
            auto &v = columns[i].values[j];
            v.type = type_uint;

            auto type = column_flags[i] & TYPE_MASK;
            assert(type < 9 || type == 0xA || type == 0xB);
            switch (type)
            {
                case 0: case 1: v.u = r.read<uint8_t>(); break;
                case 2: case 3: v.u = swap_bytes(r.read<uint16_t>()); break;
                case 4: case 5: v.u = swap_bytes(r.read<uint32_t>()); break;
                case 6: case 7: v.u = swap_bytes(r.read<uint64_t>()); break;

                case 8:
                {
                    auto u = swap_bytes(r.read<uint32_t>());
                    v.type = type_float;
                    v.f = *(float *)(&u);
                }
                break;

                case 0xA:
                {
                    v.type = type_string;
                    auto off = swap_bytes(r.read<uint32_t>()) + header.strings_offset;
                    assert(off < table_size);
                    const char *name = (char *)data + off;
                    assert(name);
                    v.s = name;
                }
                break;

                case 0xB:
                {
                    v.type = type_data;
                    auto off = swap_bytes(r.read<uint32_t>()) + header.data_offset;
                    auto sz = swap_bytes(r.read<uint32_t>());
                    assert(off + sz <= size);
                    v.d.resize(sz);
                    memcpy(v.d.data(), (char *)data + off, v.d.size());
                }
                break;
            }
        }
    }
}

//------------------------------------------------------------

const cri_utf_table::value &cri_utf_table::get_value(const std::string &name, unsigned int row) const
{
    for (auto &c: columns)
    {
        if (c.name != name)
            continue;

        if (row < c.values.size())
            return c.values[row];

        break;
    }

    return nya_memory::invalid_object<value>();
}

//------------------------------------------------------------

void cri_utf_table::debug_print() const
{
    for (auto &c: columns)
    {
        printf("%s", c.name.c_str());

        for (auto &v: c.values)
        {
            switch (v.type)
            {
                case type_uint: printf(" %llu", v.u); break;
                case type_float: printf(" %f", v.f); break;
                case type_string: printf(" <%s>", v.s.c_str()); break;
                case type_data: printf(" <data %ldb>", v.d.size()); break;
            }
        }

        printf("\n");
    }

    printf("\n");
}

//------------------------------------------------------------

bool cpk_file::open(const char *name)
{
    close();
    if (!name)
        return false;

    m_data = nya_resources::get_resources_provider().access(name);
    if (!m_data)
    {
        nya_resources::log() << "unable to open cpk file " << name << "\n";
        return false;
    }

    struct cpk_header
    {
        char sign[4];
        uint32_t unknown_ff;
        uint32_t table_size;
        uint32_t zero;
    } header;

    m_data->read_chunk(&header, sizeof(header));

    if (memcmp(header.sign, "CPK ", 4) != 0)
    {
        nya_resources::log()<<"invalid cpk file\n";
        return false;
    }

    nya_memory::tmp_buffer_ref buf(header.table_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), sizeof(header));
    cri_utf_table table(buf.get_data(), buf.get_size());
    buf.free();

    //table.debug_print();

    auto content_offset = table.get_value("ContentOffset").u;
    auto content_size = table.get_value("ContentSize").u;

    auto itoc_offset = table.get_value("ItocOffset").u;
    auto itoc_size = table.get_value("ItocSize").u;

    if (itoc_size <= sizeof(cpk_header))
        return false;

    buf.allocate(itoc_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), itoc_offset);
    cri_utf_table itoc(buf.get_data(sizeof(cpk_header)), buf.get_size() - sizeof(cpk_header));
    buf.free();
    //itoc.debug_print();

    auto &datal_buf = itoc.get_value("DataL").d;
    cri_utf_table datal(datal_buf.data(), datal_buf.size());
    //datal.debug_print();

    auto &datah_buf = itoc.get_value("DataH").d;
    cri_utf_table datah(datah_buf.data(), datah_buf.size());
    //datah.debug_print();

    return true;
}

//------------------------------------------------------------

void cpk_file::close()
{
    if (m_data)
        m_data->release();
    m_data = 0;
}

//------------------------------------------------------------
