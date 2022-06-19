-- Abusing lua as assembly
local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF

local COND_AL = 0x00  -- Always     (0b0000'0000)

local COND_EQ = 0x11  -- Equal      (0b0001'0001)
local COND_LT = 0x32  -- Less than  (0b0011'0010)
local COND_NE = 0x10  -- Not equal  (0b0001'0000)
local COND_GT = 0x30  -- Greater    (0b0011'0000)
local COND_GE = COND_GT | COND_EQ
local COND_LE = COND_LT | COND_EQ

local cmpOp1 = const(5.0)
local cmpOp2 = const(5.0)
local strPrint = const("print")

get_constant(COND_AL, 0x0000, cmpOp1)
get_constant(COND_AL, 0x0001, cmpOp2)
cmp(COND_AL, 0x0000, 0x0001)

jmp_forward(COND_GE, 5)
  stack_push(COND_AL, 0x0000)
  get_constant(COND_AL, 0x0000, strPrint)
  table_get(COND_AL, 0x0000, ENV_TABLE, 0x0000)
  call(COND_AL, 0x0000, 0, 0, 1)

ret(COND_AL, 0, 0)





