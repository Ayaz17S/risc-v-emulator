//
// Created by ayaz1 on 22-11-2025.
//
#include <iostream>
#include<vector>
#include<iomanip>
#include<cstdint>

const size_t MEMORY_SIZE = 1024*4;

class CPU{
public:
//program counter
uint32_t pc;
uint32_t regs[32];

std::vector<uint8_t> memory;

//constructor
CPU(){

pc = 0;
for(int i = 0; i < 32; i++){
regs[i] = 0;
}

memory.resize(MEMORY_SIZE,0);
//x0 is hardwired to 0 in risc -v
//we should not do anything and ensure regs[0] is never overwritten or reset it manually
regs[0] = 0;

}

void load_program(const std::vector<uint8_t>& program_code){

for(int i = 0; i < program_code.size(); i++){
if(i<MEMORY_SIZE){
memory[i] = program_code[i];}
}
std::cout<<"Program loaded. size: "<<program_code.size()<<"bytes. "<<std::endl;

}

void dump_registers(){
std::cout << "--- CPU STATE ---" << std::endl;
std::cout<<"PC: 0x"<<std::hex<<std::setw(8)<<std::setfill('0') << pc<<std::dec<<std::endl;

for(int i = 0; i < 32; i++){
std::cout<<"x" << std::setw(2)<<i<<": 0x"<<std::hex<<std::setw(8)<<std::setfill('0') << regs[i]<<" ";
    if ((i + 1) % 4 == 0) std::cout << std::endl;
}
std::cout << std::endl;
}
    uint32_t fetch() {
    if (pc + 3 >= MEMORY_SIZE) {
        std::cerr << "Error: PC out of bounds!" << std::endl;
        return 0;
    }
    uint32_t byte0 = memory[pc];
    uint32_t byte1 = memory[pc + 1];
    uint32_t byte2 = memory[pc + 2];
    uint32_t byte3 = memory[pc + 3];

    uint32_t instruction = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;

    return instruction;
}
    void execute(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    uint32_t rd = get_rd(instruction);
    uint32_t rs1 = get_rs1(instruction);
    uint32_t rs2 = get_rs2(instruction);
    uint32_t funct3 = get_funct3(instruction);

    switch (opcode) {
        case 0x13: {
            int32_t imm = get_imm_i(instruction);
            int32_t val1 = regs[rs1];

            switch (funct3) {
                case 0x00:
                    regs[rd] = val1 + imm;
                    std::cout<<"EXEC: ADDI x"<<rd<<", x "<<rs1<<", "<<imm<<std::endl;
                    break;
                default:
                    std::cout << "Unknown Funct3 for OP-IMM: " << funct3 << std::endl;
                    break;

            }
        }
            break;
        case 0x33: {
            uint32_t val1 = regs[rs1];
            uint32_t val2 = regs[rs2];
            uint32_t f7 = get_funct7(instruction);

            switch(funct3) {
                case 0x0: // ADD or SUB
                    if (f7 == 0x00) {
                        // ADD
                        regs[rd] = val1 + val2;
                        std::cout << "EXEC: ADD x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    } else if (f7 == 0x20) {
                        // SUB
                        regs[rd] = val1 - val2;
                        std::cout << "EXEC: SUB x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    }
                    break;
                case 0x4: // XOR
                    regs[rd] = val1 ^ val2;
                    std::cout << "EXEC: XOR x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                case 0x6: // OR
                    regs[rd] = val1 | val2;
                    std::cout << "EXEC: OR x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                case 0x7: // AND
                    regs[rd] = val1 & val2;
                    std::cout << "EXEC: AND x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                default:
                    std::cout << "Unknown Funct3 for R-Type: " << funct3 << std::endl;
                    break;
            }
        }
            break;
        default:
            std::cout << "Unknown Opcode: 0x" << std::hex << opcode << std::endl;
            break;
    }
    regs[0] = 0;

    // Update PC (Move to next instruction)
    pc += 4;
}

    uint32_t get_opcode(uint32_t instruction) {
    return instruction &0x7F;
}
    uint32_t get_rd(uint32_t instruction) {
    return (instruction >> 7) & 0x1F;
}
    uint32_t get_funct3(uint32_t instruction) {
    return (instruction >> 12) & 0x7;
}
    uint32_t get_rs1(uint32_t instruction) {
    return (instruction >> 15) & 0x1F;
}
    uint32_t get_rs2(uint32_t instruction) {
    return (instruction >> 20) & 0x1F;
}
    uint32_t get_funct7(uint32_t instruction) {
    return (instruction >> 25) & 0x7F;
}
    // Extract I-Type Immediate (Bits 20-31) and Sign Extend
    int32_t get_imm_i(uint32_t instruction) {
    // Cast to signed int32 so the shift preserves the sign (Arithmetic Shift)
    int32_t imm = (int32_t)instruction >> 20;
    return imm;
}
};

int main(){
CPU my_cpu;

    std::vector<uint8_t> sample_program = {
        0x93, 0x00, 0xA0, 0x00, // ADDI x1, x0, 10
        0x13, 0x01, 0x50, 0x00, // ADDI x2, x0, 5
        0xB3, 0x81, 0x20, 0x00, // ADD x3, x1, x2
        0x33, 0x82, 0x20, 0x40  // SUB x4, x1, x2
    };
my_cpu.load_program(sample_program);
//1.Fetch
// uint32_t fetched_instruction = my_cpu.fetch();
//
// my_cpu.execute(fetched_instruction);
    for (int i = 0; i < 4; i++) {
        uint32_t inst = my_cpu.fetch();

        // Safety check: stop if instruction is 0 (empty memory)
        if (inst == 0) break;

        my_cpu.execute(inst);
    }

    my_cpu.dump_registers();

// std::cout<<"Fetched instructions: 0x"<<std::hex<< fetched_instruction<<std::endl;
    // 2. Decode
    // uint32_t opcode = my_cpu.get_opcode(fetched_instruction);
    // uint32_t rd     = my_cpu.get_rd(fetched_instruction);
    // uint32_t funct3 = my_cpu.get_funct3(fetched_instruction);
    // uint32_t rs1    = my_cpu.get_rs1(fetched_instruction);

    // 3. Verify Output
    // std::cout << "Opcode : 0x" << std::hex << opcode << " (Expect 0x13 for ADDI)" << std::endl;
    // std::cout << "rd     : x"  << std::dec << rd     << " (Expect 1)"    << std::endl;
    // std::cout << "funct3 : 0x" << std::hex << funct3 << " (Expect 0)"    << std::endl;
    // std::cout << "rs1    : x"  << std::dec << rs1    << " (Expect 0)"    << std::endl;
// my_cpu.dump_registers();

return 0;
}