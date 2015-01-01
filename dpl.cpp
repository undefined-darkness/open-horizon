//
// open horizon -- undefined_darkness@outlook.com
//

#include "dpl.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include <assert.h>

//------------------------------------------------------------

void print_data(const nya_memory::memory_reader &const_reader, size_t offset, size_t size, size_t substruct_size = 0, const char *fileName = 0); //ToDo: remove

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
        uint32_t unknown_20101010;
        uint32_t flags;
        uint32_t infos_count;
        uint32_t infos_size;
    } header;

    m_data->read_chunk(&header, sizeof(header));
    if (memcmp(header.sign, "DPL\1", sizeof(header.sign)) != 0 || header.unknown_20101010 != 20101010)
       return false;

    assert(header.unknown_20101010 == 20101010);

    const bool archieved=header.flags == 2011101108;
    if (!archieved && header.flags != 2011082201)
        return false;

    nya_memory::tmp_buffer_scoped buf(header.infos_size);
    m_data->read_chunk(buf.get_data(), buf.get_size(), sizeof(header));
    nya_memory::memory_reader r(buf.get_data(), buf.get_size());

    m_infos.resize(header.infos_count);

    struct fhm_entry
    {
        char sign[4];
        uint32_t unknown_20101010;
        uint32_t flags;
        uint32_t unknown_struct_count;

        // next block zero if not arch
        uint32_t arch_unknown;
        uint32_t arch_unknown2;
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
        uint32_t unknown2;
    };

    struct unknown_struct
    {
        uint32_t idx;
        uint32_t unknown;
        uint32_t unknown_pot;
    };

    for (uint32_t i = 0; i < header.infos_count; ++i)
    {
        const fhm_entry e = r.read<fhm_entry>();

        assert(memcmp(e.sign, "FHM", 3) == 0);
        assert(e.sign[3] == (archieved ? 0 : 1));
        assert(e.unknown_20101010 == 20101010);
        assert(e.flags == header.flags);
        assert(e.idx == (archieved ? i+10000 : i));
        assert(e.offset+e.size <= (uint64_t)m_data->get_size());

        assert(e.arch_unknown_16 == (archieved ? 16 : 0));

        for (uint32_t j = 0; j < e.unknown_struct_count; ++j)
        {
            unknown_struct s = r.read<unknown_struct>();
            assert(s.idx < e.unknown_struct_count);
        }

        //if (archieved) {}

        m_infos[i].offset = e.offset;
        m_infos[i].size = e.size;
    }

    /*
    if(!m_infos.empty())
    {
        const size_t data_end=(m_infos.back().offset + m_infos.back().size);
        const size_t remained=m_data->get_size()-data_end;

        nya_memory::tmp_buffer_scoped buf(remained);
        m_data->read_chunk(buf.get_data(),remained,data_end);

        nya_memory::memory_reader r(buf.get_data(), buf.get_size());
        print_data(r,0,remained);
    }
    */

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
