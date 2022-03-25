import collections
from dis import dis
import sys
import typing
import operator
import betterproto
from lib import serializationPackage as sp

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
    return read_constant(vmdata, current)

def read_short(vmdata:sp.VMData, current: int):
    new_instruction_counter = current + 2
    # instruction_value = vmdata.instructions[value]
    first_value = vmdata.instructions[new_instruction_counter - 2] << 8
    second_value = first_value | vmdata.instructions[new_instruction_counter - 1]
    return second_value, new_instruction_counter

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
        if not type(current_stack[-1]) == float or not type(current_stack[-2]) == float:
            runtimeError("Operands must be numbers.")
            return False
        arg_b = current_stack.pop()
        arg_a = current_stack.pop()
        if (the_type == bool):
            current_stack.append(passed_func(arg_a, arg_b))
        elif (the_type == float):
            current_stack.append(current_stack, passed_func(arg_a, arg_b))
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
    vmdata.global_data = {}
    while True:
        instruction_value, instruction_counter = read_byte(vmdata, instruction_counter)
        # breakpoint()
        # print("Printing instruction counter: " + str(instruction_counter))
        # print("Executing instruction value: " + str(sp.VMDataOpcode(instruction_value)))
        if instruction_value == sp.VMDataOpcode.OP_CONSTANT:
            constant, instruction_counter = read_constant(vmdata, instruction_counter)
            data_stack.append(constant)
        elif instruction_value == sp.VMDataOpcode.OP_PRINT:
            print(data_stack.pop())
        elif instruction_value == sp.VMDataOpcode.OP_RETURN:
            break
        elif instruction_value == sp.VMDataOpcode.OP_NIL:
            data_stack.append(None)
        elif instruction_value == sp.VMDataOpcode.OP_TRUE:
            data_stack.append(True)
        elif instruction_value == sp.VMDataOpcode.OP_FALSE:
            data_stack.append(False)   
        elif instruction_value == sp.VMDataOpcode.OP_GET_GLOBAL:
            name, instruction_counter = read_string(vmdata, instruction_counter)
            if (name not in vmdata.global_data):
                runtimeError(f'Undefined variable \'{name}\'.')
                return False
            data_stack.append(vmdata.global_data[name])
        elif instruction_value == sp.VMDataOpcode.OP_DEFINE_GLOBAL:
            name, instruction_counter = read_string(vmdata, instruction_counter)
            vmdata.global_data[name] = data_stack[-1]
            data_stack.pop()
        elif instruction_value == sp.VMDataOpcode.OP_SET_GLOBAL:
            name, instruction_counter = read_string(vmdata, instruction_counter)
            if (name not in vmdata.global_data):
                runtimeError(f'Undefined variable \'{name}\'.')
                return False
            else:
                value = data_stack[-1]
                vmdata.global_data[name] = value
        elif instruction_value == sp.VMDataOpcode.OP_GET_LOCAL:  
            slot, instruction_counter = read_byte(vmdata, instruction_counter)
            data_stack.append(data_stack[slot])
        elif instruction_value == sp.VMDataOpcode.OP_SET_LOCAL: 
            slot, instruction_counter = read_byte(vmdata, instruction_counter)
            data_stack[slot] = data_stack[-1]
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
            data_stack.append(is_falsey(data_stack.pop()))
        elif instruction_value == sp.VMDataOpcode.OP_NEGATE:
            if type(data_stack[-1]) != float:
                runtimeError("Operand must be a number.")
                return False
            data_stack.append(-(data_stack.pop()))
        elif instruction_value == sp.VMDataOpcode.OP_POP:
            data_stack.pop()
        elif instruction_value == sp.VMDataOpcode.OP_EQUAL:
            b = data_stack.pop()
            a = data_stack.pop()
            data_stack.append(a == b)
        elif instruction_value == sp.VMDataOpcode.OP_GREATER:
            if binaryOp(data_stack, operator.gt, bool) == False:
                return False

        elif instruction_value == sp.VMDataOpcode.OP_LESS:
            if binaryOp(data_stack, operator.lt, bool) == False:
                return False

        elif instruction_value == sp.VMDataOpcode.OP_ADD:
            # breakpoint()
            if type(data_stack[-1]) == str and type(data_stack[-2]) == str:
                # breakpoint()
                val_2 = data_stack.pop()
                val_1 = data_stack.pop()
                data_stack.append(concatenate(val_1, val_2))
            elif type(data_stack[-1]) == float and type(data_stack[-2]) == float:
                data_stack.append(data_stack.pop() + data_stack.pop())
            else:
                runtimeError("Operands must be two numbers or two strings.")
                return False    
        elif instruction_value == sp.VMDataOpcode.OP_LOOP:
            offset, instruction_counter = read_short(vmdata, instruction_counter)
            instruction_counter -= offset
        elif instruction_value == sp.VMDataOpcode.OP_JUMP:
            offset, instruction_counter = read_short(vmdata, instruction_counter)
            instruction_counter += offset
        elif instruction_value == sp.VMDataOpcode.OP_JUMP_IF_FALSE:
            offset, instruction_counter = read_short(vmdata, instruction_counter)
            if is_falsey(data_stack[-1]):
                instruction_counter += offset

def main():
    if len(sys.argv) > 2 or len(sys.argv) < 2:
        print("Usage: plox [script]")
    elif len(sys.argv) == 2:
        run_file(sys.argv[1])

if __name__ == "__main__":
    main()
