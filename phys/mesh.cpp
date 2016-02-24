//
// open horizon -- undefined_darkness@outlook.com
//

#include "mesh.h"
#include "util/util.h"
#include "memory/memory_reader.h"

//------------------------------------------------------------

namespace phys
{
//------------------------------------------------------------

bool mesh::load(const void *data, size_t size)
{
    nya_memory::memory_reader reader(data, size);

    typedef unsigned int uint;
    typedef unsigned short ushort;

    struct colh_header
    {
        char sign[4];
        uint chunk_size;
        uint offset_to_info;
        uint unknown_zero;
        ushort count;
        ushort unknown2;
        uint offset_to_offsets;
        uint offset_to_unknown;
        ushort unknown3;
        ushort unknown4;
    };

    colh_header header = reader.read<colh_header>();
    reader.seek(0);
    //print_data(reader, 0, reader.get_remained());

    assume(header.chunk_size == reader.get_remained());
    assume(header.unknown_zero == 0);

    struct colh_info
    {
        uint unknown_32;
        uint unknown_zero;
        uint offset_to_unknown;
        uint offset_to_unknown2;
        ushort unknown;
        short unknown2;
        uint unknown_zero2;
        ushort vcount;
        ushort unknown3;
        ushort unknown4;
        ushort unknown5;
    };

    struct colh_chunk
    {
        uint offset;
        uint size;

        colh_info header;
    };

    std::vector<colh_chunk> chunks;
    chunks.resize(header.count);
    reader.seek(header.offset_to_offsets);
    for (int i = 0; i < header.count; ++i)
    {
        chunks[i].offset = reader.read<uint>();
        chunks[i].size = reader.read<uint>();
    }

    assume(header.count == 1);

    for (int i = 0; i < header.count; ++i)
    {
        colh_chunk &c = chunks[i];
        reader.seek(c.offset);

        //print_data(reader,reader.get_offset(),sizeof(colh_info));

        c.header = reader.read<colh_info>();

        //print_data(reader,reader.get_offset(),c.size-sizeof(colh_info));

        assume(c.header.unknown_32 == 32);
        assume(c.header.unknown_zero == 0);
        //for (int j = 0; j < 1; ++j)
        {
            nya_math::vec3 p;
            p.x = reader.read<float>();
            p.y = reader.read<float>();
            p.z = reader.read<float>();
            float f = reader.read<float>();
            assume(f == 1.0f);
            nya_math::vec3 p2;
            p2.x = reader.read<float>();
            p2.y = reader.read<float>();
            p2.z = reader.read<float>();
            //float f = reader.read<float>();
            float f2 = reader.read<float>();
            assume(f2 == 0);
            //test.add_point(p, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            //test.add_line(p, p2, nya_math::vec4(0.0, 1.0, 0.0, 1.0));
            nya_math::aabb b;
            b.origin = p;
            //b.delta = nya_math::vec3(f, f, f);
            b.delta = p2;

            m_box = b;
        }
    }
    
    //print_data(reader);

    return true;
}

//------------------------------------------------------------
};
