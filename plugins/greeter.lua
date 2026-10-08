-- greeter.lua — plugin de demostración de la API
function on_start()
    wm.notify("¡Hola desde Lua!")
    wm.log("greeter cargado")
end

function on_window_focused(id, title)
    wm.notify("foco: " .. title)
end

function on_window_closed(id)
    wm.notify("ventana cerrada (" .. id .. ")")
end
