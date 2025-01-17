-- Based on http://www.locklessinc.com/articles/512byte_roguelike/

local tb = require("termbox2")

local SIZEX = 96
local SIZEY = 32
local NMAX = 20

local state = "playing"
local map = {}
local mx = {}
local my = {}
local mc = {}
local mhp = {}

for y = 1, SIZEY do
	map[y] = {}
	for x = 1, SIZEX do
		map[y][x] = false
	end
end

for i = 1, NMAX do
	mx[i] = 1
	my[i] = 1
	mc[i] = 0
	mhp[i] = 0
end

local function draw_region(x1, x2, y1, y2, c)
	for y = y1, y2 do
		for x = x1, x2 do
			map[y][x] = c
		end
	end
end

local function addm(x, y, hp, c)
	for i = 1, NMAX do
		if mc[i] == 0 then
			mx[i] = x
			my[i] = y
			mhp[i] = hp
			mc[i] = c
			return
		end
	end
end

local function getm(x, y)
	for i = 1, NMAX do
		if (mx[i] == x) and (my[i] == y) then
			return i
		end
	end
	return -1
end

local function rand_place(c, hp)
	while true do
		local x = math.random(SIZEX)
		local y = math.random(SIZEY)

		if map[y][x] == "." then
			map[y][x] = c
			addm(x, y, hp, c)
			return
		end
	end
end

local function init_map()
	draw_region(1, SIZEX, 1, SIZEY, "#")

	for _ = 1, 200 do
		local x1 = math.random(2, SIZEX - 20)
		local x2 = math.random(x1 + 5, x1 + 19)
		local y1 = math.random(2, SIZEY - 8)
		local y2 = math.random(y1 + 3, y1 + 7)
		draw_region(x1, x2, y1, y2, ".")
	end
	rand_place("@", 5)

	for _ = 1, 10 do
		rand_place("m", 2)
	end
end

local function draw_screen()
	tb.clear()
	for y = 1, SIZEY do
		tb.print(1, y, table.concat(map[y]))
	end
end

local function sgn(x)
	return (x > 0) and 1 or x < 0 and -1 or 0
end

local function init()
	math.randomseed(os.time())
	init_map()
end

local function update(c, key)
	if c == "q" then
		state = "exit"
		return
	end

	for i = 1, NMAX do
		local nx = mx[i]
		local ny = my[i]

		if i == 1 then
			--if  or key == tb.KEY_ARROW_LEFT then
			if key == tb.KEY_ARROW_LEFT or ("[a147]"):match(c) then
				nx = nx - 1
			end
			if ("[d369]"):match(c) or key == tb.KEY_ARROW_RIGHT then
				nx = nx + 1
			end
			if ("[w789]"):match(c) or key == tb.KEY_ARROW_UP then
				ny = ny - 1
			end
			if ("[s123]"):match(c) or key == tb.KEY_ARROW_DOWN then
				ny = ny + 1
			end

			if map[ny][nx] == "m" then
				local j = getm(nx, ny)
				mhp[j] = mhp[j] - 1

				if mhp[j] == 0 then
					map[ny][nx] = "."
					mc[j] = 0
				end
			end
		end

		if mc[i] == "m" then
			nx = nx + sgn(mx[1] - nx)
			ny = ny + sgn(my[1] - ny)

			if map[ny][nx] == "@" then
				mhp[1] = mhp[1] - 1

				if mhp[1] <= 0 then
					state = "exit"
					return
				end
			end
		end

		if mc[i] ~= 0 and map[ny][nx] == "." then
			map[my[i]][mx[i]] = "."

			mx[i] = nx
			my[i] = ny

			map[ny][nx] = mc[i]
		end
	end
end

init()

tb.init()
tb.setcallback(tb.EVENT_KEY, function(ch, key, mod)
	update(ch, key)
end)

while state ~= "exit" do
	draw_screen()
	tb.present()
	tb.pollevent()
end

tb.shutdown()
