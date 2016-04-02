//
// open horizon -- undefined_darkness@outlook.com
//

#include "qdf.h"
#include <assert.h>
#include <string.h>

//------------------------------------------------------------

bool qdf_archive::open(const char *name)
{
    close();

    if (!name)
        return false;

    m_arch_name = name;

    FILE *arch = fopen(name, "rb");
    if (!arch)
    {
        printf("unable to open archive %s\n", name);
        return false;
    }

    struct
    {
        char sign[4];
        uint32_t unknown; //version?
        uint64_t data_split_size; //1572864000
        uint32_t file_info_count;
        uint32_t file_info_total_size;

    } header;

    //read header
    if (fread(&header, 1, sizeof(header), arch) != sizeof(header)
        || strcmp(header.sign, "QDF") != 0)
    {
        close();
        printf("invalid archive signature\n");
        return false;
    }

    //save file descriptors for all archive parts
    m_rds.push_back(arch);
    for (int i = 1; ; ++i)
    {
        char fname[255];
        sprintf(fname, "%s%d", name, i);
        FILE *f = fopen(fname, "rb");
        if (!f)
            break;

        m_rds.push_back(f);
    }

    //read file info entries
    std::vector<char> fi_buf(header.file_info_total_size);
    fread(&fi_buf[0], header.file_info_total_size, 1, arch);

    m_fis.resize(header.file_info_count);
    for (unsigned int i = 0, offset = 0; i < header.file_info_count; ++i)
    {
        qdf_file_info &info = m_fis[i];
		info.offset_to_info = offset + sizeof(header);
        offset += sizeof(uint32_t); //unknown
        info.offset = *((uint64_t*)&fi_buf[offset]);
        offset += sizeof(uint64_t);
        info.size = *((uint64_t*)&fi_buf[offset]);
        offset += sizeof(uint64_t);
        offset += sizeof(uint32_t); //padding

        //read name until \0
        for (char c = fi_buf[offset++]; c; c = fi_buf[offset++])
        {
            if (c == '\\')
                c = '/';

            info.name.push_back(c);
        }
    }

    m_part_size = header.data_split_size;

    return true;
}

//------------------------------------------------------------

void qdf_archive::close()
{
    for (auto &f: m_rds)
        fclose(f);

    m_part_size = 0;
    m_rds.clear();
    m_fis.clear();
}

//------------------------------------------------------------

const char *qdf_archive::get_file_name(int idx) const
{
    if (idx < 0 || idx >= int(m_fis.size()))
        return 0;

    return m_fis[idx].name.c_str();
}

//------------------------------------------------------------

uint64_t qdf_archive::get_file_size(int idx) const
{
    if (idx < 0 || idx >= int(m_fis.size()))
        return 0;

    return m_fis[idx].size;
}

//------------------------------------------------------------

uint64_t qdf_archive::get_file_offset(int idx) const
{
    if (idx < 0 || idx >= int(m_fis.size()))
        return 0;

    return m_fis[idx].offset;
}

//------------------------------------------------------------

uint64_t qdf_archive::get_file_info_offset(int idx) const
{
    if (idx < 0 || idx >= int(m_fis.size()))
        return 0;

    return m_fis[idx].offset_to_info;
}

//------------------------------------------------------------

int qdf_archive::get_file_idx(const char *name) const
{
    if (!name)
        return -1;

    //should use std::map instead
    for (int i = 0; i < get_files_count(); ++i)
        if (m_fis[i].name == name)
            return i;

    return -1;
}

//------------------------------------------------------------

int qdf_archive::find_file_idx(const char *name_part) const
{
    if (!name_part)
        return -1;

    for (int i = 0; i < get_files_count(); ++i)
        if (strstr(m_fis[i].name.c_str(), name_part))
            return i;

    return -1;
}

//------------------------------------------------------------

bool qdf_archive::read_file_data(int idx, void *data) const
{
    return read_file_data(idx, data, get_file_size(idx));
}

//------------------------------------------------------------

bool qdf_archive::read_file_data(int idx, void *data, uint64_t size, uint64_t offset) const
{
    if (idx < 0 || idx >= int(m_fis.size()) || !m_part_size || !data)
        return false;

    const qdf_file_info &info=m_fis[idx];

    if (size + offset > info.size)
        return false;

    offset += info.offset;

    const int fidx1 = int(offset / m_part_size);
    const int fidx2 = int((offset + size) / m_part_size);

    const size_t offset1 = size_t(offset - fidx1 * m_part_size);

#define idx_error(i) printf("failed to read file %s: qdf archive %s%d not found\n", m_fis[idx].name.c_str(), m_arch_name.c_str(), i)

    if (fidx1 >= int(m_rds.size()))
    {
        idx_error(fidx1);
        return false;
    }

    //check if file has been split between 2 parts of the archive
    if (fidx1 != fidx2)
    {
        if (fidx2 >= int(m_rds.size()))
        {
            idx_error(fidx1);
            return false;
        }

        const size_t size1 = size_t(m_part_size - offset1);

        fseek(m_rds[fidx1], offset1, SEEK_SET);
        if (fread(data, 1, size1, m_rds[fidx1]) != size1)
            return false;

        fseek(m_rds[fidx2], 0, SEEK_SET);
        const size_t size2 = size_t(size - size1);
        return fread((char *)data + size1, 1, size2, m_rds[fidx2]) == size2;
    }

    fseek(m_rds[fidx1], offset1, SEEK_SET);
    return fread(data, 1, size_t(size), m_rds[fidx1]) == size;
}

//------------------------------------------------------------
