-- coco_monitor.lua
-- Reads mc09 coco_test linear output buffer from MAME emulated RAM.
-- Exits MAME when SUITE:PASS or SUITE:FAIL is detected.
--
-- Memory layout (matches drivers/coco_test.asm):
--   $6FFE-$6FFF  WRIDX: 16-bit big-endian write pointer
--   $7000-$77FF  linear output buffer

local WRIDX_HI  = 0x0FFE
local WRIDX_LO  = 0x0FFF
local BUFBASE   = 0x1000
local OUTFILE   = "/tmp/mc09_coco_out.txt"

local cpu       = manager.machine.devices[":maincpu"]
local mem       = cpu.spaces["program"]
local read_pos  = BUFBASE
local output    = ""
local done      = false

os.remove(OUTFILE)
local fout = io.open(OUTFILE, "w")

emu.register_periodic(function()
    if done then return end

    -- Read 16-bit big-endian write pointer
    local wr_hi = mem:read_u8(WRIDX_HI)
    local wr_lo = mem:read_u8(WRIDX_LO)
    local wr    = wr_hi * 256 + wr_lo

    -- Sanity check: pointer must be >= BUFBASE and < BUFTOP
    -- Ignore reads before startup initialises WRIDX
    if wr < BUFBASE or wr > 0x7EFF then return end

    -- Drain new bytes
    while read_pos < wr do
        local ch = mem:read_u8(read_pos)
        local c  = string.char(ch)
        fout:write(c)
        fout:flush()
        output   = output .. c
        read_pos = read_pos + 1
    end

    -- Check for suite completion
    if output:find("SUITE:PASS") or output:find("SUITE:FAIL") then
        done = true
        fout:close()
        manager.machine:exit()
    end
end)
