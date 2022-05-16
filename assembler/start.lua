-- Abusing lua as assembly
local strHello = const("Hello World!")
local strPrint = const("print")
local strPogString = const("Pogger fox this working")

local ENV_TABLE = 0xFFFE
local NIL = 0xFFFF
local COND_NONE = 0x00

get_constant(COND_NONE, 0x0001, strPrint)
table_get(COND_NONE, 0x0001, ENV_TABLE, 0x0001)

get_constant(COND_NONE, 0x0002, strHello)
stack_push(COND_NONE, 0x0002)

get_constant(COND_NONE, 0x0002, strPogString)
stack_push(COND_NONE, 0x0002)

-- Call the "print" function with one argument
-- in stack and zero return
call(COND_NONE, 0x0001, 0, 0, 1)

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
