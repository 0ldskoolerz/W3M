-- keys.lua — demostración del evento on_key: muestra cada tecla en el panel
local last = ""

function on_key(code, name)
    if name ~= last then
        last = name
        wm.status("keys", "tecla: " .. name)
        wm.log("on_key code=" .. code .. " name=" .. name)
    end
end
