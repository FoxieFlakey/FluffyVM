#!/bin/env lua
local SDL = require("SDL")

local f = io.open("pipe")

local height = 600
local width = 700

local win = assert(SDL.createWindow({
  title = "heap usage",
  width = width,
  height = height
}))
local surface = win:getSurface()

function readNumber()
  local str = ""
  local chr
  repeat
    chr = assert(f:read(1))
    str = str..(chr or "")
  until chr == " "
  return assert(tonumber(str))
end

function setPixel(x, y, color)
  surface:fillRect({
    x = math.floor(x),
    y = math.floor(height - y),
    h = 1,
    w = 1
  }, color)
  --win:updateSurface()
end

function drawBar(x, y, color)
  surface:fillRect({
    x = math.floor(x),
    y = math.floor(height - y),
    h = math.floor(y),
    w = 1
  }, color)
end

function BresenhamLine(x0, y0, x1, y1, color)
  local dx = math.abs(x1 - x0);
  local sx = x0 < x1 and 1 or -1;
  local dy = -math.abs(y1 - y0);
  local sy = y0 < y1 and 1 or -1;
  local err = dx + dy;  --[[ error value e_xy ]]
  
  while true do  --[[ loop ]]
    setPixel(x0, y0, color) 
    
    if x0 == x1 and y0 == y1 then break end
    
    local e2 = 2 * err;
    if e2 >= dy then  --[[ e_xy+e_x > 0 ]]
      err = err + dy;
      x0 = x0 + sx;
    end 
    
    if e2 <= dx then --[[ e_xy+e_y < 0 ]]
      err = err + dx;
      y0 = y0 + sy;
    end 
  end 
end

function plot(x1, y1, x, y, color)
  x = math.floor(x)
  y = math.floor(y)
  x1 = math.floor(x1)
  y1 = math.floor(y1)
  BresenhamLine(x1, y1, x, y, color) 
end

local maxHeapSize = readNumber()
local t = 0
local time = 0

local prevY_usage = 0  
local prevY_gen0 = 0  
local prevY_gen1 = 0  
local prevY_gen2 = 0  

while true do
  local useSize = readNumber()
  local gen0 = readNumber()
  local gen1 = readNumber()
  local gen2 = readNumber()
  
  --local heapSize = readNumber() 
  
  surface:fillRect({
    x = t,
    y = 0,
    h = height,
    w = 20
  }, 0)
  --Heap size
  --drawBar(t, (heapSize / maxHeapSize) * height, 0x0000FF)
  
  --Current usage red
  plot(t-1, prevY_usage, t, (useSize / maxHeapSize) * height, 0xFF0000)
  prevY_usage = (useSize / maxHeapSize) * height
  
  --M13 = Objects RWLock
  --M11 = UsageLock
  
  -- Gen 2 blue
  --plot(t-1, prevY_gen2, t, (gen2 / maxHeapSize) * height, 0x0000FF)
  --prevY_gen2 = (gen2 / maxHeapSize) * height
  
  -- Gen 1 green
  --plot(t-1, prevY_gen1, t, (gen1 / maxHeapSize) * height + prevY_gen2, 0x00FF00)
  --prevY_gen1 = (gen1 / maxHeapSize) * height + prevY_gen2
  
  -- Gen 0 yellow
  --plot(t-1, prevY_gen0, t, (gen0 / maxHeapSize) * height + prevY_gen1, 0xFFA500)
  --prevY_gen0 = (gen0 / maxHeapSize) * height + prevY_gen1
  
  win:updateSurface()
  
  t = t + 1
  if t > width then
    prevY_usage = 0 
    t = 0
  end
end





















