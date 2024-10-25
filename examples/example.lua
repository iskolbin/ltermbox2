local tb = require("termbox2")

os.setlocale("", "all")

tb.init()
tb.setinputmode(tb.INPUT_ESC + tb.INPUT_MOUSE)
tb.setoutputmode(tb.OUTPUT_TRUECOLOR)

local function render(s)
	tb.clear()
	tb.print(0, 0, "hello from lua")
	tb.print(0, 1, "press any key/resize window/use mouse, press 'q' to exit")
	tb.print(0, 2, s)
	tb.present()
end

local stop = false
tb.setcallback(tb.EVENT_KEY, function(ch, key, mod)
	render(("EVENT_KEY: mod=%d key=%d ch=%s"):format(mod, key, ch))
	stop = ch == "q"
end)
tb.setcallback(tb.EVENT_RESIZE, function(w, h)
	render(("EVENT_RESIZE: w=%d h=%d"):format(w, h))
end)
tb.setcallback(tb.EVENT_MOUSE, function(x, y, key)
	render(("EVENT_MOUSE: x=%d y=%d key=%d"):format(x, y, key))
end)

render("")
repeat
	tb.pollevent()
until stop

tb.shutdown()
