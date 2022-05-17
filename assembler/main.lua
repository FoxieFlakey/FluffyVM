local global = {}

local constants = {}
local constantPointer = 0
local objectType = {}

local stack = {}
local stackPointer = 1

local prototype
local topLevelPrototype

function startPrototype()
  prototype = {
    labels = {},
    instructions = {},
    pc = 1,
    bytePointer = 1,
    prototypes = {}
  }
  stack[stackPointer] = prototype
  
  stackPointer = stackPointer + 1
end

function endPrototype()
  if prototype == topLevelPrototype then
    error("Cant end top level prototype")
  end
  
  local parent = stack[stackPointer - 2]
  stack[stackPointer - 1] = nil
  
  table.insert(parent.prototypes, prototype)
  prototype = parent
  
  stackPointer = stackPointer - 1
end

global.start_prototype = startPrototype
global.end_prototype = endPrototype

startPrototype()
topLevelPrototype = prototype

---------------------------------------------------
local opcode = {
  nop = 0x00,
  mov = 0x01,
  table_get = 0x02,
  table_set = 0x03,
  call = 0x04,
  stack_push = 0x05,
  stack_pop = 0x06,
  get_constant = 0x07,
  ret = 0x08,
  extra = 0x09,
  stack_get_top = 0x0A
}
---------------------------------------------------
function isValidReg(reg)
  return math.type(reg) == "integer" and 
         reg >= 0 and 
         reg <= 0xFFFF
end

function checkReg(reg)
  assert(isValidReg(reg), ("Invalid reg '%s'"):format(tostring(reg)))
end

function isValidCond(reg)
  return math.type(reg) == "integer" and
         reg >= 0 and
         reg <= 0xFF
end

function checkCond(reg)
  assert(isValidCond(reg), ("Invalid conditional flag '%s'"):format(tostring))
end

function isValidShort(reg)
  return math.type(reg) == "integer" and
         reg >= 0 and
         reg <= 0xFFFF
end

function checkShort(reg)
  assert(isValidShort(reg), ("Invalid short integer flag '%s'"):format(tostring(reg)))
end

function isValidConst(const)
  return type(const) == "table" and
         objectType[const] == "constant_ref" and
         constants[const]
end

function checkConst(const)
  assert(isValidConst(const), ("Invalid constant '%s'"):format(tostring(const)))
end

function emit(b)
  prototype.instructions[prototype.bytePointer] = b
  prototype.bytePointer = prototype.bytePointer + 1
end

function emitInstruction(op, cond, ...)
  assert(math.type(op) == "integer" and
         op >= 0 and op <= 0xFF)
  assert(math.type(cond) == "integer" and
         cond >= 0 and cond <= 0xFF)
  
  prototype.pc = prototype.pc + 1
  emit(op)
  emit(cond)
  local first = true
  for _num, operand in ipairs({...}) do
    assert(math.type(operand) == "integer" and
           operand >= 0 and operand <= 0xFFFF)
    local num = _num - 1
    if (_num - 1) % 3 == 0 and not first then
      prototype.pc = prototype.pc + 1
      emit(opcode.extra)
      emit(0)
    end
    
    emit(math.floor(operand / 256) % 256)
    emit(operand % 256)
    first = false
  end
  
  -- Pad
  while (prototype.bytePointer - 1) % 8 ~= 0 do
    emit(0)
  end
end

---------------------------------------------------
-- @name const
-- @param0 (string | integer) Data
-- @return constant_ref
--
-- Create constant in constant pool and return
-- constant reference which can be used
-- with get_constant
function global.const(data)  assert(constantPointer < 0xFFFF, "Number of constant limit reached")
  assert(type(data) == "string" or
         math.type(data) == "integer",
         "Expect string or integer")
  
  local ref = {}
  objectType[ref] = "constant_ref"
  constants[constantPointer] = {
    data = data,
    type = type(data)
  }
  constants[ref] = constantPointer
  
  constantPointer = constantPointer + 1
  return ref
end

---------------------------------------------------
function global.get_constant(cond, reg, const)
  checkCond(cond)
  checkReg(reg)
  checkConst(const)
  
  emitInstruction(opcode.get_constant, 0, 
                  reg, constants[const])
end
---------------------------------------------------
function global.table_get(cond, result, table, key)
  checkCond(cond)
  checkReg(result)
  checkReg(table)
  checkReg(key)
  
  emitInstruction(opcode.table_get, cond, result, table, key)
end
---------------------------------------------------
function global.stack_push(cond, reg)
  checkCond(cond)
  checkReg(reg)
  
  emitInstruction(opcode.stack_push, cond, reg)
end
---------------------------------------------------
function global.stack_get_top(cond, reg)
  checkCond(cond)
  checkReg(reg)
  
  emitInstruction(opcode.stack_get_top, cond, reg)
end
---------------------------------------------------
function global.call(cond, func, resultCount, argStart, argEnd)
  checkCond(cond)
  checkReg(func)
  checkShort(resultCount)
  checkShort(argStart)
  checkShort(argEnd)

  emitInstruction(opcode.call, cond, func, resultCount, argStart, argEnd)
end
---------------------------------------------------
function global.ret(cond, resultStart, resultEnd)
  checkCond(cond)
  checkShort(resultStart)
  checkShort(resultEnd)
  
  emitInstruction(opcode.ret, cond, resultStart, resultEnd)
end
---------------------------------------------------
function global.nop()
  emitInstruction(opcode.nop, 0)
end
---------------------------------------------------
function global.mov(cond, src, dest)
  checkCond(cond)
  checkReg(src)
  checkReg(dest)
  emitInstruction(opcode.mov, cond, src, dest)
end
---------------------------------------------------
function global.table_set(cond, table, key, data)
  checkCond(cond)
  checkReg(table)
  checkReg(key)
  checkReg(data)
  
  emitInstruction(opcode.table_set, cond, table, key, data)
end
---------------------------------------------------
function global.stack_pop(cond, result)
  checkCond(cond)
  checkReg(result)
  
  emitInstruction(opcode.stack_pop, cond, result)
end
---------------------------------------------------

local global2 = setmetatable({}, {
  __newindex = function()
    error("Modification restricted")
  end,
  
  __index = function(_, k)
    return global[k]
  end
})
loadfile("start.lua", nil, global2)()

---------------------------------------------------

print(("-"):rep(32))
print("Constant Pool")
print(("-"):rep(32))
local i = 0
while constants[i] do
  local v = constants[i]
  print(string.format("%05d %s: '%s'", i, v.type, tostring(v.data)))
  i = i + 1
end
print(("-"):rep(32))
---------------------------------------------------

function processPrototype(proto)
  print("~~~~~~~~ Prototype ~~~~~~~~")
  print("Op   Cond A         B         C")
  
  local instructions = proto.instructions
  for k, v in ipairs(instructions) do
    if (k - 1) % 8 == 0 and k ~= 1 then
      print()
    end
    io.write(string.format("0x%02X ", v))
  end
  print()
  
  for _, proto2 in pairs(proto.prototypes) do
    processPrototype(proto2)
  end
  
  print("~~~~~~~~~~~~~~~~~~~~~~~~~~~")
end

processPrototype(topLevelPrototype)
-------------------------------------------------

-- Generate bytecode

function preparePrototype(proto)
  local newInstructions = {}
  for i=0,proto.pc - 2 do
    local low, high = 0, 0
    low = proto.instructions[i * 8 + 8] +
           (proto.instructions[i * 8 + 7] * 0x100) +
           (proto.instructions[i * 8 + 6] * 0x10000) +
           (proto.instructions[i * 8 + 5] * 0x1000000)
    high = proto.instructions[i * 8 + 4] +
           (proto.instructions[i * 8 + 3] * 0x100) +
           (proto.instructions[i * 8 + 2] * 0x10000) +
           (proto.instructions[i * 8 + 1] * 0x1000000)
    --[[for j=0,3 do
      high = high + (proto.instructions[i * 8 + j + 1] * math.floor(0xFF^(3-j)))
    end
    for j=0,3 do
      low = low + (proto.instructions[i * 8 + j + 5] * math.floor(0xFF^j))
    end]]
    --print(string.format("%08X %08X (%d %d)", high, low, high, low))
    table.insert(newInstructions, {low=low, high=high})
  end
  
  proto.bytePointer = nil
  proto.pc = nil
  proto.labels = nil
  proto.instructions = newInstructions
  
  for _, proto2 in pairs(proto.prototypes) do
    preparePrototype(proto2)
  end
end

preparePrototype(topLevelPrototype)

local constantsTable = {}
for i=0,#constants do
  constantsTable[i+1] = constants[i]
end

local JSON = require("json")
local fp = io.open("bytecode.json", "w")
assert(fp)
fp:write(JSON.encode({
  --type = "fluffyvm_bytecode",
  --version = {1, 0, 0},
  constants = constantsTable,
  mainPrototype = topLevelPrototype
}))
fp:write("\n")
fp:close()

--[[
local constants = constantsTable

-- Result bytecode
local bytecode = {}

function writeU8(byte)
  table.insert(bytecode, table.pack("<B", byte))
end

function writeS8(byte)
  table.insert(bytecode, table.pack("<b", byte))
end

function writeU16(byte)
  table.insert(bytecode, table.pack("<I3", byte))
end

function writeS16(byte)
  table.insert(bytecode, table.pack("<i2", byte))
end

function writeU32(byte)
  table.insert(bytecode, table.pack("<I4", byte))
end

function writeS32(byte)
  table.insert(bytecode, table.pack("<i4", byte))
end

--------------------------------------------

--------------------------------------------

local fp = io.open("bytecode.bin", "w")
assert(fp)
fp:write(table.concat(bytecode))
fp:close()
]]


