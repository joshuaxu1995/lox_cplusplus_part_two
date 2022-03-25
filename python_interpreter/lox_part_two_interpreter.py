import collections
from dis import dis
import sys
import typing
import operator
import betterproto
from lib import serializationPackage as sp

def push(currentStack: typing.List[int], value):
    currentStack.append(value)

def pop(currentStack: typing.List[int]):
    return currentStack.pop()

def peek(currentStack: typing.List[int], distance: int):
    return currentStack[-distance]

def is_falsey(value):
    return value is None or value == False

def read_byte(vmdata:sp.VMData, current: int):
    # breakpoint()
    return (vmdata.instructions[current], current + 1)

def read_constant(vmdata:sp.VMData, current: int):
    byte_num, instruction_counter = read_byte(vmdata, current)
    current_value =  vmdata.constant_vals[byte_num]
    # breakpoint()
    if (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'object_address'):
        return (vmdata.string_map[current_value.object_address], instruction_counter)
    elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'num_val'):
        return (current_value.num_val, instruction_counter)
    elif (betterproto.which_one_of(current_value, "ValueTypes")[0] == 'bool_val'):
        return (current_value.bool_val, instruction_counter)

def read_string(vmdata:sp.VMData, current: int):
    return str(read_constant(vmdata, current))

def read_short(value, vmdata:sp.VMData):
    value = value + 2
    # instruction_value = vmdata.instructions[value]
    first_value = vmdata.instructions[value - 2] << 8
    second_value = first_value | vmdata.instructions[value - 1]
    return second_value

def runtimeError(instruction_number):
    print(f'[line {instruction_number}] in script\n')

def run_file(path: str):

    vmdata = sp.VMData()
    # Read the existing address book.
    f = open(path, "rb")
    vmdata.parse(f.read())
    f.close()

    run(vmdata)
    # print(vmdata.instructions)
    # for instruction in vmdata.instructions:
    #     if instruction == sp.VMDataOpcode.OP_CONSTANT:    
    #         print("Got constant here")
    #     else:
    #         print("Didn't get constant")

def binaryOp(current_stack, passed_func, the_type):
    while True:
        if not type(peek(current_stack, 0)) == float or not type(peek(current_stack, 1)) == float:
            runtimeError("Operands must be numbers.")
            return False
        arg_b = pop(current_stack)
        arg_a = pop(current_stack)
        if (the_type == bool):
            push(current_stack, passed_func(arg_a, arg_b))
        elif (the_type == float):
            push(current_stack, passed_func(arg_a, arg_b))
        break
    return True

def concatenate(str1: str, str2: str):
    total_str = str1 + str2
    return total_str

def build_string_map(string_data):
    string_map = {}
    for string_datum in string_data:
        string_map[string_data[string_datum].address] = string_datum
    
    return string_map

def run(vmdata: sp.VMData):

    instruction_counter = 0
    data_stack = []
    vmdata.string_map = build_string_map(vmdata.strings_at_addresses)
    while True:
        instruction_value, instruction_counter = read_byte(vmdata, instruction_counter)
        # breakpoint()
        # print("Printing instruction counter: " + str(instruction_counter))
        if instruction_value == sp.VMDataOpcode.OP_CONSTANT:
            constant, instruction_counter = read_constant(vmdata, instruction_counter)
            push(data_stack, constant)
        elif instruction_value == sp.VMDataOpcode.OP_PRINT:
            print(pop(data_stack))
        elif instruction_value == sp.VMDataOpcode.OP_RETURN:
            break
        elif instruction_value == sp.VMDataOpcode.OP_LOOP:
            offset = read_short()
            instruction_counter -= offset
        elif instruction_value == sp.VMDataOpcode.OP_NIL:
            push(data_stack, None)
        elif instruction_value == sp.VMDataOpcode.OP_TRUE:
            push(data_stack, True)
        elif instruction_value == sp.VMDataOpcode.OP_FALSE:
            push(data_stack, False)   
        elif instruction_value == sp.VMDataOpcode.OP_GET_LOCAL:  
            slot, _ = read_byte(vmdata, instruction_counter)
            push(data_stack, slot)
        elif instruction_value == sp.VMDataOpcode.OP_SET_LOCAL: 
            slot, _ = read_byte(vmdata, instruction_counter)
            data_stack[slot] = peek(data_stack, 0)
        elif instruction_value == sp.VMDataOpcode.OP_SUBTRACT:
            if (binaryOp(data_stack, operator.sub, float) == False):
                return False
        elif instruction_value == sp.VMDataOpcode.OP_MULTIPLY:
            if (binaryOp(data_stack, operator.mul, float) == False):
                return False
        elif instruction_value == sp.VMDataOpcode.OP_DIVIDE:
            if (binaryOp(data_stack, operator.truediv, float) == False):
                return False
        elif instruction_value == sp.VMDataOpcode.OP_NOT:
            push(data_stack, is_falsey(pop(data_stack)))
        elif instruction_value == sp.VMDataOpcode.OP_NEGATE:
            if not peek(0).isNumber():
                runtimeError("Operand must be a number.")
                return False
            push(-(pop(data_stack)))
        elif instruction_value == sp.VMDataOpcode.OP_POP:
            pop(data_stack)
        elif instruction_value == sp.VMDataOpcode.OP_EQUAL:
            b = pop(data_stack)
            a = pop(data_stack)
            push(data_stack, a == b)
        elif instruction_value == sp.VMDataOpcode.OP_GREATER:
            if binaryOp(data_stack, operator.ge, bool) == False:
                return False

        elif instruction_value == sp.VMDataOpcode.OP_LESS:
            if binaryOp(data_stack, operator.le, bool) == False:
                return False

        elif instruction_value == sp.VMDataOpcode.OP_ADD:
            # breakpoint()
            if type(peek(data_stack, 0)) == str and type(peek(data_stack, 1)) == str:
                # breakpoint()
                val_1 = pop(data_stack)
                val_2 = pop(data_stack)
                push(data_stack, concatenate(val_1, val_2))
            elif type(peek(data_stack, 0)) == float and type(peek(data_stack, 1)) == float:
                push(data_stack, pop(data_stack) + pop(data_stack))
            else:
                runtimeError("Operands must be two numbers or two strings.")
                return False    
    

def main():
    if len(sys.argv) > 2 or len(sys.argv) < 2:
        print("Usage: plox [script]")
    elif len(sys.argv) == 2:
        run_file(sys.argv[1])

if __name__ == "__main__":
    main()
