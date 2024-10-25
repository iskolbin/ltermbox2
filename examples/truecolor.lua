local tb = require("termbox2")

os.setlocale("", "all")

tb.init()
tb.setoutputmode(tb.OUTPUT_TRUECOLOR)

local function hsv2rgb(h, s, v)
	local C = v * s
	local m = v - C
	local r, g, b = m, m, m
	if h == h then
		local h_ = (h % 1.0) * 6
		local X = C * (1 - math.abs(h_ % 2 - 1))
		C, X = C + m, X + m
		if h_ < 1 then
			r, g, b = C, X, m
		elseif h_ < 2 then
			r, g, b = X, C, m
		elseif h_ < 3 then
			r, g, b = m, C, X
		elseif h_ < 4 then
			r, g, b = m, X, C
		elseif h_ < 5 then
			r, g, b = X, m, C
		else
			r, g, b = C, m, X
		end
	end
	return r, g, b
end

local variateSaturation = false
local running = true
local xx = 0

local SYMBOLS = "qwertyuioplkjhgfdsazxcvbnmQWERTYUIOPLKJHGFDSAZXCVBNM1234567890"

local function render()
	tb.clear()
	local w, h = tb.width(), tb.height()
	for x = 0, w - 1 do
		local H = ((x + xx) % w) / (w - 1)
		for y = 0, h - 1 do
			local V = variateSaturation and 1 or y / (h - 1)
			local S = variateSaturation and y / (h - 1) or 1
			local rf, gf, bf = hsv2rgb(H, S, V)
			local rb, gb, bb = hsv2rgb(H, V, S)
			local fg = math.floor(rf * 0xff) * 0x10000 + math.floor(gf * 0xff) * 0x100 + math.floor(bf * 0xff)
			local bg = math.floor(rb * 0xff) * 0x10000 + math.floor(gb * 0xff) * 0x100 + math.floor(bb * 0xff)
			fg = fg == 0 and tb.HI_BLACK or fg
			bg = bg == 0 and tb.HI_BLACK or bg
			local i = math.random(SYMBOLS:len())
			tb.setcell(x, y, SYMBOLS:sub(i, i), fg, bg)
		end
	end
	tb.present()
end

tb.setcallback(tb.EVENT_RESIZE, function()
	render()
end)

tb.setcallback(tb.EVENT_KEY, function(_, key)
	if key == tb.KEY_ESC then
		running = false
	elseif key == tb.KEY_SPACE then
		variateSaturation = not variateSaturation
		render()
	end
end)

while running do
	render()
	xx = xx + 1
	tb.peekevent(100)
end

tb.shutdown()
