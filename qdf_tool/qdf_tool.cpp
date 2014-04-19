//
// open horizon -- undefined_darkness@outlook.com
//

#include "qdf.h"
#include <cstdlib>
#include <map>
#include <sys/stat.h>

//------------------------------------------------------------

static void create_path(const char *dir)
{
    if (!dir)
        return;

    std::string tmp(dir);
    for (char *p = &tmp[1]; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp.c_str(), S_IRWXU);
            *p = '/';
        }
    }
    //mkdir(tmp, S_IRWXU);
}

//------------------------------------------------------------

bool write_file(const char *name, const void *buf, size_t size)
{
    if (!name || !buf)
        return false;

    FILE *f = fopen(name, "wb");
    if (!f)
    {
        printf("unable to write file %s", name);
        return false;
    }

    fwrite(buf, size, 1, f);
    fclose(f);

    return true;
}

//------------------------------------------------------------

const char *names[] = { "datafile.qdf", "datafile.qdf1", "datafile.qdf2",
                        "datafile.qdf3", "datafile.qdf4", "datafile.qdf5" };

//------------------------------------------------------------

template<typename t>
bool write_to_arch(uint64_t offset, const t& value, t&old_value)
{
    const size_t arch_part_size = 1572864000; //sorry
    const int idx = int(offset / arch_part_size);
    offset -= idx * arch_part_size;

    FILE *arch = fopen(names[idx], "rb+");
    if (!arch)
    {
        printf("unable ot open arch %s", names[idx]);
        return false;
    }
    fseek(arch,offset, SEEK_SET);
    fread(&old_value, sizeof(t), 1, arch);
    fseek(arch,offset, SEEK_SET);
    fwrite(&value, sizeof(t), 1, arch);
    fclose(arch);

    printf("wrote to arch %s at offset %lld\n", names[idx], offset);

    return true;
}

//------------------------------------------------------------

int main(int argc, const char* argv[])
{
    if (argc <= 1)
    {
        printf("qdf_tool list_files\n");
        printf("\n");
        printf("qdf_tool extract_file\n");
        printf("qdf_tool extract_file output_path\n");
        printf("\n");
        printf("qdf_tool extract_all\n");
        printf("qdf_tool extract_all output_path\n");
        printf("\n");
        printf("qdf_tool write_float filename offset value\n");
        printf("qdf_tool write_float filename offset value count\n");
        printf("\n");
        printf("qdf_tool write_uint filename offset value\n");
        printf("qdf_tool write_uint filename offset value count\n");
        printf("\n");
        printf("qdf_tool restore\n");
        return -1;
    }

    qdf_archive qdf;
    if (!qdf.open(names[0]))
        return -1;

    //read backup
    std::map<uint64_t,uint32_t> backup;
    FILE *bfile = fopen("backup.tmp","rb");
    if (bfile)
    {
        int count;
        fread(&count, 4, 1, bfile);
        for (int i=0; i < count; ++i)
        {
            uint64_t offset;
            uint32_t value;

            fread(&offset, sizeof(offset), 1, bfile);
            fread(&value, sizeof(value), 1, bfile);
            backup[offset] = value;
        }
        fclose(bfile);
    }

    //list files in archive
    if (strcmp(argv[1], "list_files") == 0)
    {
        const int count = qdf.get_files_count();
        for(int i = 0; i < count; ++i)
            printf("%s\n", qdf.get_file_name(i));
        return 0;
    }

    //extract file from archive
    if (strcmp(argv[1], "extract_file") == 0)
    {
        if (argc <= 2)
        {
            printf("qdf_tool extract_file\n");
            printf("qdf_tool extract_file output_path\n");
            return -1;
        }

        //even part of the file's name acceptable
        const int idx = qdf.get_file_idx(argv[2]);
        if (!idx)
        {
            printf("file not found in arch: %s\n", argv[2]);
            return -1;
        }

        std::vector<char> buf(qdf.get_file_size(idx));
        qdf.read_file_data(idx, &buf[0]);

        const char *name = qdf.get_file_name(idx);
        for (const char *c = name; *c; ++c)
            if(*c=='/')
                name = c + 1;

        if (argc>3)
        {
            std::string fname(argv[3]);
            if(fname[fname.size() - 1] != '/')
                fname.push_back('/');

            create_path(fname.c_str());
            fname += name;
            write_file(fname.c_str(), &buf[0], buf.size());
        }
        else
            write_file(name, &buf[0], buf.size());

        return 0;
    }

    //extract all files in archive
    if (strcmp(argv[1], "extract_all") == 0)
    {
        if (argc <= 1)
        {
            printf("qdf_tool extract_all\n");
            printf("qdf_tool extract_all output_path\n");
            return -1;
        }

        std::vector<char> buf;

        const int count = qdf.get_files_count();
        for (int i = 0; i < count; ++i)
        {
            buf.resize(qdf.get_file_size(i));
            qdf.read_file_data(i, &buf[0]);

            std::string fname;
            if(argc>2)
            {
                fname = argv[2];
                if(fname[fname.size() - 1] != '/')
                    fname.push_back('/');
            }

            fname += qdf.get_file_name(i);

            create_path(fname.c_str());
            write_file(fname.c_str(), &buf[0], buf.size());

            printf("%s\n", qdf.get_file_name(i));
        }
        return 0;
    }

    //override value in archive, e.g. to test how does it affect the game
    if (strcmp(argv[1], "write_float") == 0 || strcmp(argv[1], "write_uint") == 0)
    {
        if (argc <= 4)
        {
            printf("qdf_tool write_float filename offset value\n");
            printf("qdf_tool write_float filename offset value count\n");
            printf("qdf_tool write_uint filename offset value\n");
            printf("qdf_tool write_uint filename offset value count\n");
            return -1;
        }

        int count = argc <= 5 ? 1 : atoi(argv[5]); //write count is optional

        const int idx = qdf.get_file_idx(argv[2]);
        if (!idx)
        {
            printf("file not found in arch: %s\n", argv[2]);
            return -1;
        }

        auto offset = qdf.get_file_offset(idx);

        const int fidx1 = int(offset/qdf.get_part_size());
        qdf.close(); //so it could be overwritten

        size_t user_offset = atoi(argv[3]);
        unsigned int old_value;
        if (strcmp(argv[1], "write_float") == 0)
        {
            float value = atof(argv[4]);
            for (int i = 0; i < count; ++i)
            {
                uint64_t off = offset + user_offset + i*4;
                write_to_arch(off, value, *((float *)&old_value));
                printf("writing %f instead of %f in archive %s file %s at offset %ld\n",
                       value, *((float *)&old_value), argv[2], names[fidx1], user_offset + i*4);

                if (backup.find(off) == backup.end())
                    backup[off] = old_value;
            }
        }
        else if (strcmp(argv[1], "write_uint") == 0)
        {
            unsigned int value = atoi(argv[4]);
            for (int i = 0; i < count; ++i)
            {
                uint64_t off = offset + user_offset + i*4;
                write_to_arch(off, value, old_value);
                printf("writing %d instead of %d in archive %s file %s at offset %ld\n",
                       value, old_value, argv[2], names[fidx1], user_offset + i*4);

                if(backup.find(off) == backup.end())
                    backup[off] = old_value;
            }
        }
        else
            printf("error argv[1]\n");
    }

    //restore from backup
    if (strcmp(argv[1], "restore") == 0)
    {
        int count=0;
        for (auto &v: backup)
        {
            uint64_t offset = v.first;
            uint32_t value = v.second, old_value;

            write_to_arch(offset, value, old_value);
            //printf("test %f %f\n",*((float *)&value),*((float *)&old_value));
            ++count;
        }
        printf("restored %d values\n", count);
        backup.clear();
    }

    //save backup file
    bfile = fopen("backup.tmp", "wb");
    if (bfile)
    {
        int count = int(backup.size());
        fwrite(&count, 4, 1, bfile);
        for (auto &v: backup)
        {
            uint64_t offset = v.first;
            uint32_t value = v.second;

            fwrite(&offset, sizeof(offset), 1, bfile);
            fwrite(&value, sizeof(value), 1, bfile);
        }
        fclose(bfile);
    }

	return 0;
}

//------------------------------------------------------------
