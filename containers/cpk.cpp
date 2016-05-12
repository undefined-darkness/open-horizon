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

inline void read_value(nya_memory::memory_reader &r, int type, cri_utf_table::value &v, uint32_t strings_offset, uint32_t data_offset)
{
    v.type = cri_utf_table::type_uint;
    assert(type < 9 || type == 0xA || type == 0xB);
    switch (type)
    {
        case 0: case 1: v.u = r.read<uint8_t>(); break;
        case 2: case 3: v.u = swap_bytes(r.read<uint16_t>()); break;
        case 4: case 5: v.u = swap_bytes(r.read<uint32_t>()); break;
        case 6: case 7: r.skip(4); v.u = swap_bytes(r.read<uint32_t>()); break;

        case 8: v.type = cri_utf_table::type_float; v.f = swap_bytes(r.read<float>()); break;

        case 0xA:
        {
            v.type = cri_utf_table::type_string;
            auto off = swap_bytes(r.read<uint32_t>()) + strings_offset;
            auto prev_off = r.get_offset();
            r.seek(off);
            assert(r.get_data());
            v.s = (char *)r.get_data();
            r.seek(prev_off);
        }
        break;

        case 0xB:
        {
            v.type = cri_utf_table::type_data;
            auto off = swap_bytes(r.read<uint32_t>()) + data_offset;
            auto sz = swap_bytes(r.read<uint32_t>());
            if (!sz)
                break;

            auto prev_off = r.get_offset();
            r.seek(off);
            assert(r.get_data());
            v.d.resize(sz);
            memcpy(v.d.data(), r.get_data(), v.d.size());
            r.seek(prev_off);
        }
        break;
    }
}

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

    const char *table_name = (char *)data + header.strings_offset + header.table_name;
    name = table_name ? table_name : "";

    std::vector<uint8_t> column_flags(header.num_colums);
    columns.resize(header.num_colums);

    num_rows = (int)header.num_rows;

    enum
    {
        STORAGE_MASK = 0xf0,
        STORAGE_NONE = 0x00,
        STORAGE_ZERO = 0x10,
        STORAGE_CONSTANT = 0x30,
        STORAGE_PERROW = 0x50,

        TYPE_MASK = 0x0f
    };

    for (uint32_t i = 0; i < header.num_colums; ++i)
    {
        column_flags[i] = r.read<uint8_t>();
        if (!column_flags[i])
        {
            r.skip(3);
            column_flags[i] = r.read<uint8_t>();
        }

        auto off = swap_bytes(r.read<uint32_t>());

        off += header.strings_offset;
        assert(off < table_size + utf_hoff);
        const char *name = (char *)data + off;
        assert(name);
        columns[i].name = name;

        if ((column_flags[i] & STORAGE_MASK) == STORAGE_CONSTANT)
        {
            value v;
            read_value(r, column_flags[i] & TYPE_MASK, v, header.strings_offset, header.data_offset);
            columns[i].values.resize(header.num_rows, v);
        }
    }

    for (uint32_t j = 0; j < header.num_rows; ++j)
    {
        r.seek(header.rows_offset + j * header.row_length);

        for (uint32_t i = 0; i < header.num_colums; ++i)
        {
            auto storage = column_flags[i] & STORAGE_MASK;
            if (storage != STORAGE_PERROW)
                continue;

            columns[i].values.resize(header.num_rows);
            read_value(r, column_flags[i] & TYPE_MASK, columns[i].values[j], header.strings_offset, header.data_offset);
        }
    }
}

//------------------------------------------------------------

const cri_utf_table::column &cri_utf_table::get_column(const std::string &name) const
{
    for (auto &c: columns) if (c.name == name) return c;
    return nya_memory::invalid_object<column>();
}

//------------------------------------------------------------

const cri_utf_table::value &cri_utf_table::get_value(const std::string &name, int row) const
{
    if (name.empty() || row < 0)
        return nya_memory::invalid_object<value>();

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
    printf("table: %s\n", name.c_str());
    for (auto &c: columns)
    {
        char type = c.values.empty() ? ' ' : c.values.front().type;

        printf("%s <%c> [%d]", c.name.c_str(), type, (int)c.values.size());

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

    auto data = nya_resources::get_resources_provider().access(name);
    if (!data)
    {
        nya_resources::log() << "unable to open cpk file " << name << "\n";
        return false;
    }

    return open(data);
}

//------------------------------------------------------------

bool cpk_file::open(nya_resources::resource_data *data)
{
    close();
    m_data = data;
    if (!m_data)
        return false;

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
    //auto content_size = table.get_value("ContentSize").u;
    auto align = table.get_value("Align").u;

    auto itoc_offset = table.get_value("ItocOffset").u;
    auto itoc_size = table.get_value("ItocSize").u;

    if (itoc_size <= sizeof(cpk_header))
        return false;

    buf.allocate(itoc_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), itoc_offset);
    cri_utf_table itoc(buf.get_data(sizeof(cpk_header)), buf.get_size() - sizeof(cpk_header));
    buf.free();
    //itoc.debug_print();

    const std::vector<char> *data_buf[2] = {&(itoc.get_value("DataL").d), &(itoc.get_value("DataH").d)};
    for (auto &d: data_buf)
    {
        cri_utf_table data(d->data(), d->size());
        //data.debug_print();

        for (int i = 0; i < data.num_rows; ++i)
        {
            entry e;
            e.id = (uint32_t)data.get_value("ID", i).u;
            e.size = (uint32_t)data.get_value("FileSize", i).u;
            assert(e.size == (uint32_t)data.get_value("ExtractSize", i).u); //ToDo?
            m_entries.push_back(e);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const entry &a, const entry &b){ return a.id < b.id; });

    uint32_t offset = (uint32_t)content_offset;
    uint32_t idx = 0;
    for (auto &e: m_entries)
    {
        assume(e.id == idx++);

        e.offset = offset;
        offset += e.size;
        if (align > 0 && (e.size % align))
            offset += (align - (e.size % align));

        assert(e.offset + e.size <= m_data->get_size());
    }

    return true;
}

//------------------------------------------------------------

void cpk_file::close()
{
    if (m_data)
        m_data->release();
    m_entries.clear();
    m_data = 0;
}

//------------------------------------------------------------

uint32_t cpk_file::get_file_size(int idx) const
{
    if (idx < 0 || idx >= get_files_count())
        return 0;

    return m_entries[idx].size;
}

//------------------------------------------------------------

bool cpk_file::read_file_data(int idx, void *data) const
{
    return read_file_data(idx, data, get_file_size(idx));
}

//------------------------------------------------------------

bool cpk_file::read_file_data(int idx, void *data, uint32_t size, uint32_t offset) const
{
    if (idx < 0 || idx >= get_files_count() || !m_data || !data)
        return false;

    const auto &e = m_entries[idx];
    if (offset + size > e.size)
        return false;

    return m_data->read_chunk(data, size, e.offset + offset);
}

//------------------------------------------------------------
