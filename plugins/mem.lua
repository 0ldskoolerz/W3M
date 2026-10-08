-- mem.lua — uso de RAM en el panel, actualizado cada 5 s (estilo lxpanel)
local last = 0

function on_tick()
    local now = os.time()
    if now - last < 5 then return end
    last = now
    local f = io.open("/proc/meminfo", "r")
    if not f then return end
    local total, avail
    for line in f:lines() do
        local k, v = line:match("^(%w+):%s+(%d+)")
        if k == "MemTotal" then total = tonumber(v) end
        if k == "MemAvailable" then avail = tonumber(v) end
    end
    f:close()
    if total and avail then
        local used_mb = math.floor((total - avail) / 1024)
        local total_mb = math.floor(total / 1024)
        wm.status("mem", string.format("RAM %d/%dM", used_mb, total_mb))
    end
end
