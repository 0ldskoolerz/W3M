-- apps.lua — lista las aplicaciones corriendo en el panel de estado
local last = ""

function on_tick()
    local names = {}
    for _, id in ipairs(wm.window_ids()) do
        local t = wm.window_title(id)
        if t ~= "" then table.insert(names, t) end
    end
    local s = #names .. " apps"
    if #names > 0 then s = s .. ": " .. table.concat(names, ", ") end
    if s ~= last then
        last = s
        if #names == 0 then wm.status("apps", "sin apps") end
    end
    wm.status("apps", #names == 0 and "sin apps" or s)
end
