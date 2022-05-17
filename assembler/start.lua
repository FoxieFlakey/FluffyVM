-- Abusing lua as assembly
local strHello = const("Hello World!")
local strPrint = const("print")
local strReturnString = const("return_string")
local strPogString = const("Pogger fox this working")

local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF
local COND_NONE = 0x00

get_constant(COND_NONE, 0x0001, strHello)
stack_push(COND_NONE, 0x0001)

get_constant(COND_NONE, 0x0001, strPogString)
stack_push(COND_NONE, 0x0001)

-- Call the "return_string" function
get_constant(COND_NONE, 0x0001, strReturnString)
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)
call(COND_NONE, 0x0001, 1, 0, 0)

-- Call the "print" string with all argument from 
-- 0 to top of stack
get_constant(COND_NONE, 0x0001, strPrint)
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)
-- Check opcodes.txt for what number 1 represent
call(COND_NONE, 0x0001, 0, 0, 1)

stack_pop(COND_NONE, NIL)

-- Print all string except the last string
-- returned by `return_string`
-- Check opcodes.txt for what number 3 represent
call(COND_NONE, 0x0001, 0, 0, 4)

ret(COND_NONE, 0, 0)

start_prototype()
nop()
mov(0, 0, 0)
table_set(0, 0, 0, 0)

start_prototype()
stack_push(0, 0)
stack_pop(0, 0)
stack_get_top(0, 0)
end_prototype()

end_prototype()
