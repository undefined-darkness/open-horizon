//
// open horizon -- undefined_darkness@outlook.com
//

#pragma once

#include "extensions/zip_resources_provider.h"
#include "formats/tga.h"
#include "memory/tmp_buffer.h"
#include "math/scalar.h"
#include "containers/dpl.h"
#include "renderer/texture.h"
#include "util/xml.h"
#include <stdint.h>

namespace renderer
{
//------------------------------------------------------------

class aircraft_information
{
public:
    struct color_info
    {
        std::string name;
        int coledit_idx;
        unsigned char colors[6][3];
    };

    struct mod_info: public color_info
    {
        std::string zip_name, tex_name, spec_name;
        unsigned int decal_crc32, specular_crc32;

        nya_scene::texture load_texture(std::string name) const
        {
            nya_resources::zip_resources_provider z(zip_name.c_str());
            return renderer::load_texture(z, name.c_str());
        }

        template<int size> void load_netimg(std::string name, netimg<size> & out) const
        {
            //ToDo: should be unified with load_texture

            nya_resources::zip_resources_provider z(zip_name.c_str());
            auto data = load_resource(z.access(name.c_str()));
            if (!data.get_size())
                return;

            nya_formats::tga tga;
            if (tga.decode_header(data.get_data(), data.get_size()))
            {
                const void *img_data = tga.data;
                if (tga.rle)
                {
                    nya_memory::tmp_buffer_ref tmp(tga.uncompressed_size);
                    tga.decode_rle(tmp.get_data());
                    img_data = tmp.get_data();
                    data.free();
                    data = tmp;
                }

                if (tga.vertical_flip)
                    tga.flip_vertical(img_data, (void*)img_data);

                nya_render::bitmap_rgb_to_bgr((unsigned char *)img_data, tga.width, tga.height, tga.channels);
                out = netimg<size>(tga.width, tga.height, tga.channels, (unsigned char *)img_data);
            }
            data.free();
        }

    };

    struct engine_info
    {
        float radius = 0.0f, dist = 0.0f, yscale = 1.0f;
        nya_math::vec2 tvc;
    };

    struct info
    {
        std::string name;
        nya_math::vec3 camera_offset;
        std::vector<color_info> colors;
        std::vector<mod_info> color_mods;
        std::string sound, voice;
        std::vector<engine_info> engines;
    };

    info *get_info(const char *name)
    {
        if (!name)
            return 0;

        std::string name_str(name);
        std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);

        for (auto &i: m_infos)
        {
            if (i.name == name_str)
                return &i;
        }

        return 0;
    }

    static aircraft_information &get()
    {
        static aircraft_information info("target/Information/AircraftInformation.AIN");
        static bool once = true;
        if (once)
        {
            aircraft_information info5("target/Information/AircraftInformationC05.AIN"); //ToDo
            *info.get_info("tnd4") = *info5.get_info("tnd4");

            auto &f14d = *info.get_info("f14d");
            if (f14d.colors.size() > 1)
                std::swap(f14d.colors.back().coledit_idx, f14d.colors[f14d.colors.size() - 2].coledit_idx);

            auto &av8b = *info.get_info("av8b");
            av8b.colors.resize(3);
            av8b.colors.back().coledit_idx = 3;

            pugi::xml_document doc;
            if (load_xml("aircrafts.xml", doc))
            {
                auto root = doc.first_child();
                for (pugi::xml_node ac = root.child("aircraft"); ac; ac = ac.next_sibling("aircraft"))
                {
                    auto inf = info.get_info(ac.attribute("id").as_string(""));
                    if (!inf)
                        continue;

                    for (int i = 0; i < (int)inf->colors.size(); ++i)
                    {
                        auto d = ac.child(("decal" + std::to_string(i)).c_str());
                        if (!d)
                            continue;

                        inf->colors[i].name = d.attribute("name").as_string();

                        if (!d.first_child())
                            continue;

                        for (int j = 0; j < 6; ++j)
                        {
                            auto c = d.child(("color" + std::to_string(j)).c_str());
                            if (c)
                            {
                                inf->colors[i].colors[j][0] = c.attribute("r").as_int();
                                inf->colors[i].colors[j][1] = c.attribute("g").as_int();
                                inf->colors[i].colors[j][2] = c.attribute("b").as_int();
                            }
                        }
                    }

                    for (pugi::xml_node eng = ac.child("engine"); eng; eng = eng.next_sibling("engine"))
                    {
                        engine_info ei;
                        ei.radius = eng.attribute("radius").as_float();
                        ei.dist = eng.attribute("dist").as_float();
                        ei.yscale = eng.attribute("yscale").as_float(1.0f);
                        ei.tvc.x = ei.tvc.y = eng.attribute("tvc_xy").as_float();
                        ei.tvc.y = eng.attribute("tvc_y").as_float(ei.tvc.y);
                        inf->engines.push_back(ei);
                    }
                }
            }

            auto custom_decals = list_files("decals/");
            for (auto &d: custom_decals)
            {
                nya_resources::zip_resources_provider zprov;
                if (!zprov.open_archive(d.c_str()))
                    continue;

                pugi::xml_document doc;
                if (!load_xml(zprov.access("info.xml"), doc))
                {
                    zprov.close_archive();
                    continue;
                }

                auto root = doc.first_child();

                mod_info m;
                m.name = root.attribute("name").as_string();
                m.zip_name = d;
                m.coledit_idx = root.attribute("base").as_int();

                auto img = root.child("image");
                if (img)
                {
                    m.tex_name = img.attribute("file").as_string();
                    m.decal_crc32 = zprov.get_resource_crc32(zprov.get_resource_idx(m.tex_name.c_str()));
                }

                auto simg = root.child("specular");
                if (simg)
                {
                    m.spec_name = simg.attribute("file").as_string();
                    m.specular_crc32 = zprov.get_resource_crc32(zprov.get_resource_idx(m.spec_name.c_str()));
                }

                zprov.close_archive();

                auto plane_id = root.attribute("plane").as_string();

                auto pi = info.get_info(plane_id);
                if (!pi)
                {
                    printf("plane '%s' not found for decal mod '%s'\n", plane_id, d.c_str());
                    continue;
                }

                if (m.coledit_idx < 0 || m.coledit_idx >= pi->colors.size())
                {
                    printf("invalid base idx %d for decal mod '%s', base count: %d\n", m.coledit_idx, d.c_str(), (int)pi->colors.size());
                    continue;
                }

                memcpy(m.colors, pi->colors[m.coledit_idx].colors, sizeof(m.colors));

                for (int i = 0; i < sizeof(m.colors) / sizeof(m.colors[0]); ++i)
                {
                    auto c = root.child(("color" + std::to_string(i)).c_str());
                    if (!c)
                        continue;

                    m.colors[i][0] = c.attribute("r").as_int();
                    m.colors[i][1] = c.attribute("g").as_int();
                    m.colors[i][2] = c.attribute("b").as_int();
                }

                pi->color_mods.push_back(m);
            }

            once = false;
        }

        return info;
    }

    aircraft_information(const char *name)
    {
        //ToDo: don't determine aircraft order by camera offsets file

        auto r = load_resource("Camera/AdjustedCameraOffset.txt");

        std::vector<std::string> values;
        std::string tmp;
        for (size_t i = 0; i < r.get_size(); ++i)
        {
            char c=((char *)r.get_data())[i];
            if (c=='\n' || c=='\r')
            {
                if (tmp.empty())
                    continue;

                auto t = tmp.substr(0,7);

                if (t !="float\t.")
                {
                    tmp.clear();
                    continue;
                }

                tmp = tmp.substr(t.length());

                for (size_t j = 0; j < tmp.size(); ++j)
                {
                    std::string type = tmp.substr(0, j);
                    if (tmp[j] == '\t')
                    {
                        tmp = tmp.substr(j + 1);
                        break;
                    }
                }

                std::string plane_name = tmp.substr(0,4);
                std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);

                auto info = get_info(plane_name.c_str());
                if (!info)
                {
                    m_infos.resize(m_infos.size() + 1);
                    m_infos.back().name = plane_name;
                    info = &m_infos.back();
                    //printf("%s\n",plane_name.c_str());
                }

                auto d = tmp.find(':');
                auto vname = tmp.substr(d + 1);
                tmp = tmp.substr(5, d - 1 - 5);
                float v = atof(vname.c_str());

                if (tmp == "AroundLook.Offset.X") info->camera_offset.x = v;
                if (tmp == "AroundLook.Offset.Y") info->camera_offset.y = v;
                if (tmp == "AroundLook.Offset.Z") info->camera_offset.z = v;

                //printf("%s=%f\n",tmp.c_str(),v);

                tmp.clear();
                continue;
            }

            tmp.push_back(c);
        }

        r.free();

        r = load_resource(name);
        nya_memory::memory_reader reader(r.get_data(),r.get_size());

        struct ain_header
        {
            char sign[4];
            uint32_t unknown;
            uint32_t count;
            uint32_t zero;
        };

        auto header = reader.read<ain_header>();
        if (memcmp(header.sign, "AIN", 3) == 0)
        {
            struct ain_entry
            {
                uint16_t color_num;
                uint16_t plane_num;
                uint32_t unknown[39];
                uint8_t colors[6][4];
                uint32_t unknown2[6];
            };

            std::vector<ain_entry> entries(header.count);
            for (auto &e: entries)
            {
                e = reader.read<ain_entry>();
                auto plane_idx = int(e.plane_num)-1;
                assert(plane_idx >= 0);

                if (plane_idx >= m_infos.size())
                    continue;

                auto &info = m_infos[plane_idx];
                auto color_idx = int(e.color_num)-1;
                if (color_idx >= (int)info.colors.size())
                     info.colors.resize(color_idx + 1);

                auto &c = info.colors[color_idx];
                c.coledit_idx = -1;
                for (int i = 0; i < 6; ++i)
                {
                    c.colors[i][0] = e.colors[i][3];
                    c.colors[i][1] = e.colors[i][2];
                    c.colors[i][2] = e.colors[i][1];
                }
            }
        }
        r.free();

        //ToDo

        std::map<std::string,std::vector<int> > ints;
        std::map<std::string, std::pair<std::string, std::string> > sounds;

        dpl_file dp;
        dp.open("datapack.bin");

        std::string buf;
        for (int i = 0; i < dp.get_files_count(); ++i)
        {
            buf.resize(dp.get_file_size(i));
            dp.read_file_data(i, &buf[0]);
            assert(buf.size()>16);
            if (strncmp(buf.c_str(), ";DPL_P_", 7) != 0)
                continue;

            std::string plane_name = buf.substr(7,4);
            std::transform(plane_name.begin(), plane_name.end(), plane_name.begin(), ::tolower);

            if (buf[7+4] == '\n')
            {
                auto s0 = buf.find("ound/");     if (s0 == std::string::npos) continue;
                auto e0 = buf.find('\x9', s0);    if (e0 == std::string::npos) continue;
                auto s1 = buf.find("ound/", e0); if (s1 == std::string::npos) continue;
                auto e1 = buf.find('\x9', s1);    if (e1 == std::string::npos) continue;

                auto test = sounds[plane_name] = std::make_pair('s' + buf.substr(s0, e0 - s0), 's' + buf.substr(s1, e1 - s1));
                test = test;
            }
            else if (buf[7+4] == '_' && buf[7+4+1] == 'T')
            {
                int idx = atoi(&buf[7+4+2]);
                ints[plane_name].push_back(idx);
            }
        }

        dp.close();

        for (auto &info: m_infos)
        {
            auto &in = ints[info.name];

            auto s = sounds.find(info.name);
            if (s != sounds.end())
                info.sound = s->second.first, info.voice = s->second.second;

            int last_idx = 0;
            for (size_t i = 0; i < info.colors.size(); ++i)
            {
                //assert(i < in.size());
                if (i < in.size())
                {
                    info.colors[i].coledit_idx = in[i];
                    last_idx = in[i];
                }
                else
                    info.colors[i].coledit_idx = ++last_idx;
            }
        }
    }

private:
    std::vector<info> m_infos;
};

//------------------------------------------------------------
}
