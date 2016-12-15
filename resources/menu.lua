--Open Horizon menu

function init()
    --ToDo
end

function on_set_screen(screen)

    if screen == "main" then
        set_title("MAIN MENU")

        if next(get_missions()) then
            add_entry("Mission", "mode=ms", "screen=mission_select", "multiplayer=no")
        end

        add_entry("Deathmatch", "mode=dm", "screen=map_select", "multiplayer=no")
        add_entry("Team deathmatch", "mode=tdm", "screen=map_select", "multiplayer=no")
        add_entry("Free flight", "mode=ff", "screen=map_select", "multiplayer=no")
        add_entry("Multiplayer", "screen=mp");
        add_entry("Settings", "screen=settings");
        --add_entry("Aircraft viewer", "mode=none", "screen=ac_view")
        add_entry("Exit", "exit")
        return
    end

    if screen == "map_select" then
        set_title("LOCATION")
        for i,entry in ipairs(get_locations()) do add_entry(entry.name, "map="..entry.id, "screen=ac_select") end
        return
    end

    if screen == "mission_select" then
        set_title("MISSION")
        send_event("reset_mission_desc");
        for i,entry in ipairs(get_missions()) do add_entry(entry.name, "mission="..entry.id, "screen=ac_select", "update_mission_desc") end
        return
    end

    if screen == "ac_select" then
        set_title("AIRCRAFT")

        ac_list = get_aircrafts("fighter", "multirole", "attacker", "bomber")

        --ToDo
    end

    --ToDo
end
