-- bluetooth.lua — estado de Bluetooth en el panel (bluetoothctl)
local last = 0
local cached = "..."
local scanning = false

local function read_bt()
    -- powered?
    local f = io.popen("bluetoothctl show 2>/dev/null")
    if not f then return nil end
    local out = f:read("*a") or ""
    f:close()
    if out == "" then return "sin bluetooth" end
    local powered = out:find("Powered: yes") ~= nil
    if not powered then return "BT off" end
    -- dispositivos conectados
    local fc = io.popen("bluetoothctl info 2>/dev/null")
    local connected = false
    if fc then
        local info = fc:read("*a") or ""
        fc:close()
        connected = info:find("Connected: yes") ~= nil
    end
    if scanning then return "BT escaneando..." end
    return connected and "BT conectado" or "BT on"
end

function on_tick()
    local now = os.time()
    if now - last < 5 then return end
    last = now
    cached = read_bt() or "sin bluetooth"
    wm.status("bt", cached)
end

function on_key(code, name)
    if name == "F7" then
        -- F7: escanea dispositivos bluetooth cercanos y lista los 4 primeros
        wm.notify("BT: escaneando 5s...")
        scanning = true
        os.execute("bluetoothctl scan on 2>/dev/null")
        -- el scan es async; esperamos y listamos
        local f = io.popen("sleep 5 && bluetoothctl devices 2>/dev/null")
        if not f then return end
        local devs = {}
        for line in f:lines() do
            local name = line:match("^Device %x%x:%x%x:%x%x:%x%x:%x%x:%x%x (.+)$")
            if name and #devs < 4 then table.insert(devs, name) end
        end
        f:close()
        os.execute("bluetoothctl scan off 2>/dev/null")
        scanning = false
        if #devs == 0 then
            wm.notify("BT: sin dispositivos encontrados")
        else
            wm.notify("BT: " .. table.concat(devs, " | "))
        end
    end
end
