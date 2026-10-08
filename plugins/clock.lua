-- clock.lua — fecha y hora en el panel de estado (secciones "clock" y "date")
local last = ""

function on_tick()
    local t = os.date("%H:%M:%S")
    if t ~= last then
        last = t
        wm.status("clock", t)
        wm.status("date", os.date("%d/%m/%Y"))
    end
end

function on_start()
    wm.log("clock iniciado")
end
