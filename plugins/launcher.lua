-- launcher.lua — abre una ventana nueva con F2 (demostración de on_key)
local n = 0

function on_start()
    wm.log("launcher listo: pulsa F2 para abrir una ventana")
end

function open_demo()
    n = n + 1
    local id = wm.create_window("Lua demo " .. n, 220, 120)
    if id >= 0 then
        wm.notify("ventana #" .. id .. " creada desde Lua")
    end
end

function on_key(code, name)
    if name == "F2" then open_demo() end
end
