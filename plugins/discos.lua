-- discos.lua — uso de discos en el panel + F6 lista dispositivos montables
local last = 0
local cached = "..."

local function read_root()
    local f = io.popen("df -h / 2>/dev/null")
    if not f then return nil end
    f:read("*l")   -- header
    local line = f:read("*l") or ""
    f:close()
    local size, used, pct = line:match("^%S+%s+(%S+)%s+(%S+)%s+%S+%s+(%d+)%%")
    if size then return used .. "/" .. size .. " (" .. pct .. "%)" end
    return nil
end

local function list_removable()
    -- discos/particiones montables: no montadas y con filesystem
    local f = io.popen("lsblk -rno NAME,SIZE,TYPE,MOUNTPOINT,FSTYPE 2>/dev/null")
    if not f then return {} end
    local out = {}
    for line in f:lines() do
        local name, size, typ, mount, fstype = line:match("^(%S+)%s+(%S+)%s+(%S+)%s*(%S*)%s*(%S*)")
        if fstype and fstype ~= "" and (mount == "" or mount == nil)
           and (typ == "part" or typ == "disk") and not name:match("^zram") then
            table.insert(out, name .. "(" .. size .. "," .. fstype .. ")")
        end
    end
    f:close()
    return out
end

function on_tick()
    local now = os.time()
    if now - last < 10 then return end
    last = now
    cached = read_root() or "df?"
    wm.status("discos", cached)
end

function on_key(code, name)
    if name == "F5" then
        -- F5: lista discos/particiones montables (no montadas)
        local parts = list_removable()
        if #parts == 0 then
            wm.notify("discos: nada montable")
        else
            wm.notify("montables: " .. table.concat(parts, " "))
        end
    elseif name == "F6" then
        -- F6: desmonta todo lo montado por el usuario bajo /media o /mnt
        local f = io.popen("findmnt -rno TARGET 2>/dev/null")
        if not f then return end
        local unmounted = 0
        for line in f:lines() do
            if line:match("^/media/") or line:match("^/mnt/") then
                -- umount puede requerir permisos; udiskctl es preferible
                local ok = os.execute("umount " .. line .. " 2>/dev/null")
                if not ok then
                    os.execute("udisksctl unmount -b $(findmnt -rno SOURCE " .. line .. ") 2>/dev/null")
                end
                unmounted = unmounted + 1
            end
        end
        f:close()
        if unmounted == 0 then
            wm.notify("discos: nada que desmontar")
        else
            wm.notify("discos: " .. unmounted .. " desmontado(s)")
        end
    end
end
