-- red.lua — estado de la conexion de red en el panel (NetworkManager)
local last = 0
local cached = "..."

local function read_network()
    local f = io.popen("nmcli -t -f TYPE,STATE,CONNECTION device status 2>/dev/null")
    if not f then return nil end
    local best, best_name
    for line in f:lines() do
        local typ, state, conn = line:match("^(%w+):(%w+):(.*)$")
        if typ and state == "connected" then
            -- prioriza ethernet sobre wifi
            if typ == "ethernet" or (typ == "wifi" and not best) then
                best = typ
                best_name = conn ~= "" and conn or typ
            end
        end
    end
    f:close()
    return best, best_name
end

function on_tick()
    local now = os.time()
    if now - last < 5 then return end
    last = now
    local typ, name = read_network()
    if typ == "ethernet" then
        cached = "RED: " .. (name or "eth")
    elseif typ == "wifi" then
        cached = "WIFI: " .. (name or "?")
    else
        cached = "sin red"
    end
    wm.status("red", cached)
end

function on_key(code, name)
    -- F11: lista las redes wifi disponibles en la línea de estado
    if name == "F11" then
        local f = io.popen("nmcli -t -f SSID device wifi list 2>/dev/null")
        if not f then return end
        local ssids = {}
        for line in f:lines() do
            if line ~= "" and #ssids < 4 then table.insert(ssids, line) end
        end
        f:close()
        if #ssids == 0 then
            wm.notify("wifi: no se encontraron redes")
        else
            wm.notify("wifi: " .. table.concat(ssids, " | "))
        end
    end
end
