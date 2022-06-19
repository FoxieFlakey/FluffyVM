-- Abusing lua as assembly
local NIL = 0xFFFF
local ENV_TABLE = 0xFFFE
local SELF_FUNCTION = 0xFFFD

local COND_AL = 0x00  -- Always     (0b0000'0000)

local COND_EQ = 0x11  -- Equal      (0b0001'0001)
local COND_LT = 0x32  -- Less than  (0b0011'0010)
local COND_NE = 0x10  -- Not equal  (0b0001'0000)
local COND_GT = 0x30  -- Greater    (0b0011'0000)
local COND_GE = COND_GT | COND_EQ
local COND_LE = COND_LT | COND_EQ

local strPrint = const("print")
local strGetCPUTime = const("getCPUTime")
local strMessage = const("Benchmark took ")
local strMessage2 = const(" ms\n")
local strMessage3 = const("Running benchmark\n")

--[[
reg 0x0000 = tmp

reg 0x0001 = startTime
reg 0x0002 = endTime
reg 0x0003 = payload
reg 0x0004 = getCPUTime
reg 0x0005 = print
reg 0x0006 = timeSpent
]]

get_constant(COND_AL, 0x0000, strGetCPUTime)
table_get(COND_AL, 0x0004, ENV_TABLE, 0x0000)

get_constant(COND_AL, 0x0000, strPrint)
table_get(COND_AL, 0x0005, ENV_TABLE, 0x0000)

get_constant(COND_AL, 0x0000, strMessage3)
stack_push(COND_AL, 0x0000)
call(COND_AL, 0x0005, 0, 2)

-- Load payload
load_prototype(COND_AL, 0x0003, 0)

-- Record start time
call(COND_AL, 0x0004, 1, 0)
stack_pop(COND_AL, 0x0001)

--------- Payload --------
call(COND_AL, 0x0003, 0, 0)
--------------------------

-- Record end time
call(COND_AL, 0x0004, 1, 0)
stack_pop(COND_AL, 0x0002)

-- Calculate time spent
sub(COND_AL, 0x0006, 0x0002, 0x0001)

-- Printing result
get_constant(COND_AL, 0x0000, strMessage)
stack_push(COND_AL, 0x0000)

-- Add time spent
stack_push(COND_AL, 0x0006)

get_constant(COND_AL, 0x0000, strMessage2)
stack_push(COND_AL, 0x0000)
call(COND_AL, 0x0005, 0, 4)

ret(COND_AL, 0, 0)

-- Factorial test (Prototype Index 0)
start_prototype()
  --[[
  reg 0x0001 = factorial function
  reg 0x0002 = i
  reg 0x0003 = max
  reg 0x0004 = 1
  reg 0x0005 = 16
  ]]

  local intI = const(0)
  local intMax = const(1)
  local int1 = const(1)
  local int16 = const(16)

  load_prototype(COND_AL, 0x0001, 0)
  
  get_constant(COND_AL, 0x0002, intI)
  get_constant(COND_AL, 0x0003, intMax)
  get_constant(COND_AL, 0x0004, int1)
  get_constant(COND_AL, 0x0005, int16)

  -- Loop begin
    stack_push(COND_AL, 0x0005)
    call(COND_AL, 0x0001, 1, 1)

    get_constant(COND_AL, 0x0000, strPrint)
    table_get(COND_AL, 0x0000, ENV_TABLE, 0x0000)
    call(COND_AL, 0x0000, 0, 1)

  add(COND_AL, 0x0002, 0x0002, 0x0004)
  cmp(COND_AL, 0x0002, 0x0003)
  jmp_backward(COND_LT, 9)

  ret(COND_AL, 0, 0)

  -- Factorial function (Prototype Index 0)
  start_prototype()
    --[[
    reg 0x0001 = 0
    reg 0x0002 = n
    reg 0x0003 = n2
    reg 0x0004 = 1
    ]]
    local int0 = const(0)

    stack_pop(COND_AL, 0x00002)
    get_constant(COND_AL, 0x0001, int0)
    get_constant(COND_AL, 0x0004, int1)

    cmp(COND_AL, 0x0002, 0x0001)
      mov(COND_EQ, 0x0000, 0x0004)
      ret(COND_EQ, 0, 1)
    --Not equals branch
      sub(COND_AL, 0x0003, 0x0002, 0x0004)
      stack_push(COND_AL, 0x0003)
      call(COND_AL, SELF_FUNCTION, 1, 1)
      stack_pop(COND_AL, 0x0003)
      mul(COND_AL, 0x0000, 0x0002, 0x0003)
      ret(COND_AL, 0, 1)
    --Exit if
  end_prototype()
end_prototype()



