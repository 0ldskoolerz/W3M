-- audio.lua — volumen y estado de audio en el panel (PulseAudio/PipeWire)
-- F8 = silenciar/desilenciar, F9 = bajar volumen, F10 = subir volumen
local last = 0
local cached = "..."

local function read_volume()
    -- pactl funciona con PulseAudio y PipeWire (pac-compatible)
    local f = io.popen("pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null")
    if not f then return nil, nil end
    local out = f:read("*a") or ""
    f:close()
    local pct = out:match("(%d+)%%")
    local fm = io.popen("pactl get-sink-mute @DEFAULT_SINK@ 2>/dev/null")
    local mute = false
    if fm then
        local m = fm:read("*a") or ""
        fm:close()
        mute = m:find("yes") ~= nil
    end
    return pct and tonumber(pct), mute
end

local function set_volume(delta)
    local pct = read_volume()
    if not pct then return end
    local target = math.max(0, math.min(100, pct + delta))
    os.execute(string.format("pactl set-sink-volume @DEFAULT_SINK@ %d%%", target))
    cached = target .. "%"
end

local function toggle_mute()
    os.execute("pactl set-sink-mute @DEFAULT_SINK@ toggle")
    local _, mute = read_volume()
    cached = mute and "MUTE" or cached
end

function on_tick()
    local now = os.time()
    if now - last < 3 then return end
    last = now
    local pct, mute = read_volume()
    if pct then
        if mute then cached = "MUTE (" .. pct .. "%)"
        else cached = "VOL " .. pct .. "%" end
    else
        cached = "sin audio"
    end
    wm.status("audio", cached)
end

function on_key(code, name)
    if name == "F8" then toggle_mute()
    elseif name == "F9" then set_volume(-5)
    elseif name == "F10" then set_volume(5) end
    -- refresca inmediatamente tras la acción
    local pct, mute = read_volume()
    if pct then
        if mute then cached = "MUTE (" .. pct .. "%)"
        else cached = "VOL " .. pct .. "%" end
    end
    wm.status("audio", cached)
end
