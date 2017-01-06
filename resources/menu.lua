--Open Horizon menu

function init()
    init_var("name", "PLAYER");
    init_var("address", "127.0.0.1");
    init_var("port", "8001");
end

function on_set_screen(screen)

    if screen == "main" then
        set_title("MAIN MENU")
        set_bkg(0)

        if next(get_missions()) then
            add_entry("Mission", "mode=ms", "screen=mission_select", "multiplayer=no")
        end

        add_entry("Deathmatch", "mode=dm", "screen=map_select", "multiplayer=no")
        add_entry("Team deathmatch", "mode=tdm", "screen=map_select", "multiplayer=no")
        add_entry("Free flight", "mode=ff", "screen=map_select", "multiplayer=no")
        add_entry("Multiplayer", "screen=mp");
        add_entry("Settings", "screen=settings");
        --add_entry("Aircraft viewer", "mode=none", "screen=ac_view")

        if next(get_campaigns()) then
            add_entry("Campaign", "screen=campaign_select", "multiplayer=no")
        end

        add_entry("Exit", "exit")

        return
    end

    if screen == "map_select" then
        set_title("LOCATION")
        set_bkg(2)

        for i,entry in ipairs(get_locations()) do
            add_entry(entry.name, "map="..entry.id, "screen=ac_select") 
        end

        return
    end

    if screen == "mission_select" then
        set_title("MISSION")
        set_bkg(2)

        send_event("reset_mission_desc");
        for i,entry in ipairs(get_missions()) do
            add_entry(entry.name, "mission="..entry.id, "screen=ac_select", "update_mission_desc")
        end

        return
    end

    if screen == "campaign_select" then
        set_title("CAMPAIGN")
        set_bkg(1)

        for i,entry in ipairs(get_campaigns()) do
            add_entry(entry.name, "campaign="..entry.id, "load_campaign")
        end

        return
    end

    if screen == "ac_select" then
        set_title("AIRCRAFT")
        set_bkg(1)

        if get_var("mode") == "ff" or get_var("mode") == "ms" then
            ac_list = get_aircrafts("fighter", "multirole", "attacker", "bomber")
        else
            ac_list = get_aircrafts("fighter", "multirole")
        end

        for i,entry in ipairs(ac_list) do
            add_entry(entry.name, "aircraft="..entry.id, "screen=color_select")
        end

        return
    end

    if screen == "color_select" then
        set_title("AIRCRAFT COLOR")
        set_bkg(1)

        for i,name in ipairs(get_aircraft_colors(get_var("aircraft"))) do
            add_entry(name, "color="..(i-1), "start")
        end

        return
    end

    if screen == "mp" then
        set_title("MULTIPLAYER")
        set_bkg(2)

        --add_entry("Internet servers", "screen=mp_inet", "multiplayer=client")
        --add_entry("Local network servers", "screen=mp_local", "multiplayer=client")
        add_entry("Connect to address", "screen=mp_connect", "multiplayer=client")
        add_entry("Start server", "screen=mp_create", "multiplayer=server")

        return
    end

    if screen == "mp_connect" then
        set_title("CONNECT TO SERVER")
        set_bkg(2)
        set_history("main", "mp")

        add_entry("Name: ")
        add_input("name")

        add_entry("Address: ")
        add_input("address")
        add_entry("Port: ")
        add_input("port", true)
        add_entry("Connect", "connect")

        return
    end
end

function on_event(event)
    --print("on_event "..event)
end

function on_set_var(name, value)
    --print("on_set_var "..name.." = "..value)
end
