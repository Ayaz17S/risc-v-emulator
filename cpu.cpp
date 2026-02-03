//
// Created by ayaz1 on 22-11-2025.
//
#include <iostream>
#include<vector>
#include<iomanip>
#include<cstdint>
#include <fstream>
#include<sstream>
#include <limits>
#include "assembler.h"

const size_t MEMORY_SIZE = 1024*4;

class CPU{
public:
//program counter
uint32_t pc;
uint32_t regs[32];

std::vector<uint8_t> memory;
    //csr constants

    const uint32_t MTVEC = 0x305;
    const uint32_t MEPC = 0x341;
    const uint32_t MCAUSE = 0x342;  // Why did we jump?
    const uint32_t MSTATUS = 0x300;

    uint32_t csrs[4096];


//constructor
CPU(){

pc = 0;
for(int i = 0; i < 32; i++){
regs[i] = 0;
}

    for(int i=0; i<4096; i++) csrs[i] = 0;

memory.resize(MEMORY_SIZE,0);
//x0 is hardwired to 0 in risc -v
//we should not do anything and ensure regs[0] is never overwritten or reset it manually
regs[0] = 0;
    regs[2]= MEMORY_SIZE;

}

    void trigger_trap(uint32_t cause) {
    std::cout << "--- TRAP TRIGGERED (Cause: " << cause << ") ---" << std::endl;
    //
    // csrs[MEPC] = pc;

    // 2. Set the cause
    csrs[MCAUSE] = cause;

    // 3. Jump to the Trap Handler (defined in MTVEC)
    pc = csrs[MTVEC];
}

    void return_from_trap() {
    std::cout << "--- MRET: Returning from Trap ---" << std::endl;
    pc = csrs[MEPC]; // Restore PC
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
    bool load_from_file(const std::string& file_name) {
    std::ifstream file(file_name, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << file_name << std::endl;
        return false;
    }
    std::streamsize size = file.tellg();
    file.seekg(0,std::ios::beg);

    if (size > MEMORY_SIZE) {
        std::cerr << "Warning: File size (" << size << ") is larger than memory (" << MEMORY_SIZE << "). Truncating." << std::endl;
        size = MEMORY_SIZE;
    }
    file.read((char*)memory.data(), size);
    file.close();
    std::cout << "Successfully loaded " << size << " bytes from " << file_name << std::endl;
    return true;
}
    void execute(uint32_t instruction) {
    uint32_t opcode = get_opcode(instruction);
    uint32_t rd = get_rd(instruction);
    uint32_t rs1 = get_rs1(instruction);
    uint32_t rs2 = get_rs2(instruction);
    uint32_t funct3 = get_funct3(instruction);

    uint32_t next_pc = pc + 4;
    switch (opcode) {
        case 0x73: {
            uint32_t f12 = (instruction>>20)&0xFFF;

            if (f12==0) {
                //std::cout << "[DEBUG] ECALL at PC: " << pc << std::endl;
                csrs[MEPC] = pc + 4;
                //std::cout << "[DEBUG] Saved MEPC: " << csrs[MEPC] << std::endl;
                trigger_trap(11);
                next_pc = pc;
            }
            else if (f12==0x302) {
               // std::cout << "[DEBUG] MRET executing. MEPC is: " << csrs[MEPC] << std::endl;
                return_from_trap();
                next_pc = pc;
            }
            else if (funct3==1) {
                uint32_t csr_addr = (instruction >> 20) & 0xFFF; // Top 12 bits
                uint32_t temp = csrs[csr_addr];
                csrs[csr_addr] = regs[rs1]; // Write to CSR
                regs[rd] = temp;            // Read old value to rd
                std::cout << "EXEC: CSRRW x" << rd << ", csr[0x" << std::hex << csr_addr << "]" << std::dec << std::endl;
            }
        }
            break;



        case 0x13: {
            int32_t imm = get_imm_i(instruction);
            int32_t val1 = regs[rs1];
            uint32_t uval1 = (uint32_t) regs[rs1];
            int shamt = imm &0x1F;

            switch (funct3) {
                case 0x0:
                    regs[rd] = val1 + imm;
                    std::cout<<"EXEC: ADDI x"<<rd<<", x "<<rs1<<", "<<imm<<std::endl;
                    break;
                    case 0x1://SLLI
                    regs[rd] = val1<<shamt;
                    std::cout << "EXEC: SLLI x" << rd << ", x" << rs1 << ", " << shamt << std::endl;
                    break;
                case 0x2://SLTI
                    regs[rd] =((int32_t)val1<imm)?1:0;
                    std::cout << "EXEC: SLTI x" << rd << ", x" << rs1 << ", " << imm << std::endl;
                    break;

                case 0x3://SLTIU
                    regs[rd] = (val1 < (uint32_t)imm) ? 1 : 0;
                    std::cout << "EXEC: SLTIU x" << rd << ", x" << rs1 << ", " << imm << std::endl;
                    break;
                case 0x5:
                    if ((instruction>>30)&1) {
                        regs[rd] = val1 >> shamt;
                        std::cout << "EXEC: SRAI x" << rd << ", x" << rs1 << ", " << shamt << std::endl;
                    }else {
                        regs[rd] = uval1 >> shamt;
                        std::cout << "EXEC: SRLI x" << rd << ", x" << rs1 << ", " << shamt << std::endl;
                    }
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
            int shamt = val2 & 0x1F;
            if (f7==0x01) {
                switch (funct3) {
                    case 0x0: //MUL
                        regs[rd] = val1*val2;
                        std::cout << "EXEC: MUL x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                        break;
                    case 0x1://MULH
                        regs[rd]=(uint32_t)(((int64_t)(int32_t)val1*(int64_t)(int32_t)val2)>>32);
                        std::cout << "EXEC: MULH x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                        break;
                    case 0x4://DIV
                        if (val2 == 0) regs[rd] = -1;
                        else if ((int32_t)val1==INT32_MIN && (int32_t)val2==-1) regs[rd] = val1;
                        else regs[rd] = (int32_t)val1 / (int32_t)val2;
                        std::cout << "EXEC: DIV x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                        break;
                    case 0x6://REM
                        if (val2 == 0) regs[rd] = -1;
                        else if ((int32_t)val1 == INT32_MIN && (int32_t)val2 == -1) regs[rd] = 0;
                        else regs[rd] = (int32_t)val1 % (int32_t)val2;
                        std::cout << "EXEC: REM x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                        break;
                    default:
                        std::cout << "Unknown M-Extension Funct3: " << funct3 << std::endl;

                }
                break;
            }

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
                case 0x1://SLL
                    regs[rd] = val1 << shamt;
                    std::cout << "EXEC: SLL x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                case 0x2://SLT
                    regs[rd]=((int32_t)val1<(int32_t)val2)?1:0;
                    std::cout << "EXEC: SLT x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                case 0x3://SLTU
                    regs[rd] =(val1<val2)?1:0;
                    std::cout << "EXEC: SLTU x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    break;
                case 0x5:
                    if ((instruction>>30)&1) {
                        regs[rd] = (int32_t)val1 >> shamt;
                        std::cout << "EXEC: SRA x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
                    } else {
                        regs[rd] = val1 >> shamt;
                        std::cout << "EXEC: SRL x" << rd << ", x" << rs1 << ", x" << rs2 << std::endl;
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
        case 0x03: {
            int32_t imm = get_imm_i(instruction);
            uint32_t address = regs[rs1]+imm;
            uint32_t loaded_val =0;

            uint8_t b0 =memory[address];
            uint16_t h0 =memory[address] | (memory[address+1]<<8);

            switch (funct3) {
                case 0x0://LB
                    loaded_val =(int8_t)b0;
                    std::cout << "EXEC: LB x" << rd << "..." << std::endl;
                    break;
                case 0x1://LH
                    loaded_val = (int16_t)h0;
                    std::cout << "EXEC: LH x" << rd << "..." << std::endl;
                    break;
                case 0x02://LW
                    loaded_val = mem_read_32(address);
                    std::cout << "EXEC: LW x" << rd << "..." << std::endl;
                    break;
                case 0x04://LBU
                    loaded_val = b0;
                    std::cout << "EXEC: LBU x" << rd << "..." << std::endl;
                    break;
                case 0x05://LHU
                    loaded_val = h0;std::cout << "EXEC: LHU x" << rd << "..." << std::endl;
                    break;
                default:
                    std::cout << "Unknown Load Funct3: " << funct3 << std::endl;
            }
            regs[rd] = loaded_val;
        }
            break;
        case 0x23: {
            int32_t imm = get_imm_s(instruction);
            uint32_t address = regs[rs1]+imm;
            uint32_t val= regs[rs2];

            if (address==0xF0 && funct3==0x0) {
                std::cout << "UART OUT: " << (char)(val & 0xFF) << std::endl;
                break;
            }

            switch (funct3) {
                case 0x0: // SB (Store Byte)
                    memory[address] = val & 0xFF;
                    std::cout << "EXEC: SB..." << std::endl;
                    break;
                case 0x1: // SH (Store Half)
                    memory[address]   = val & 0xFF;
                    memory[address+1] = (val >> 8) & 0xFF;
                    std::cout << "EXEC: SH..." << std::endl;
                    break;
                case 0x2: // SW (Store Word)
                    mem_write_32(address, val);
                    std::cout << "EXEC: SW..." << std::endl;
                    break;
                default:
                    std::cout << "Unknown Store Funct3: " << funct3 << std::endl;
            }
        }
            break;

            break;
        case 0x63: {
            int32_t imm = get_imm_b(instruction);
            uint32_t val1 = regs[rs1];
            uint32_t val2 = regs[rs2];
            bool take_branch = false;

            switch (funct3) {
                case 0x0: //BEQ
                    take_branch = (val1 == val2);
                    std::cout<< "EXEC: BEQ x"<<rs1<<", x"<<rs2<<", offset"<<imm<<(take_branch? "[TAKEN]":" [NOT TAKEN]")<<std::endl;
                    break;
                case 0x1: //BNE
                    take_branch = (val1 != val2);
                    std::cout << "EXEC: BNE x" << rs1 << ", x" << rs2 << ", offset " << imm << (take_branch ? " [TAKEN]" : " [NOT TAKEN]") << std::endl;
                    break;

                case 0x4: //BLT
                    take_branch = ((int32_t)val1<(int32_t)val2);
                    std::cout << "EXEC: BLT x" << rs1 << ", x" << rs2 << ", offset " << imm << std::endl;
                    break;
                case 0x5://BGE
                    take_branch = ((int32_t)val1 >= (int32_t)val2);
                    std::cout << "EXEC: BGE x" << rs1 << ", x" << rs2 << ", offset " << imm << std::endl;
                    break;
                default:
                    std::cout << "Unknown Branch Funct3: " << funct3 << std::endl;
            }
            if (take_branch) {
                next_pc = pc + imm; // Jump!
            }
        }
            break;
        case 0x6F: //JAL
            {
            int32_t imm = get_imm_j(instruction);

            regs[rd] = pc +4;

            next_pc = pc + imm;
            std::cout<< "EXEC: JAL x"<<rd<<", offset"<<imm<<std::endl;

        }
    break;


    case 0x67://JALR
            {
        int32_t imm = get_imm_i(instruction);
        uint32_t val1 = regs[rs1];

        regs[rd] = pc +4;

        next_pc = (val1 + imm)&0xFFFFFFFE;
        std::cout << "EXEC: JALR x" << rd << ", x" << rs1 << ", " << imm << std::endl;
    }
            break;

        case 0x37://LUI
        {
            int32_t imm = get_imm_u(instruction);
            regs[rd]= imm;
            std::cout<<"EXEC: LUI x"<<rd<<", "<<std::hex<<imm<<std::dec<<std::endl;
        }
            break;
        case 0x17://AUIPC
        {
            int32_t imm = get_imm_u(instruction);
            regs[rd]= pc+imm;
            std::cout<<"EXEC: AUIPC x"<<rd<<", offset"<<std::hex << imm << std::dec << std::endl;

        }
        break;

        default:
            std::cout << "Unknown Opcode: 0x" << std::hex << opcode << std::endl;
            break;
    }
    regs[0] = 0;

    // Update PC (Move to next instruction)
    pc = next_pc;
};

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
    uint32_t mem_read_32(uint32_t address) {
    if (address + 3 >= MEMORY_SIZE) {
        std::cout << "Error: Memory Read Out of Bounds at " << address << std::endl;
        return 0;
    }
    return (memory[address+3] << 24) | (memory[address+2] << 16) |
               (memory[address+1] << 8)  | memory[address];
}

    void mem_write_32(uint32_t address, uint32_t value) {

    if (address ==0x000000F0) {
        std::cout<<"UART OUT: "<<(char)(value &0xFF)<<std::endl;
        return;
    }

    if (address + 3 >= MEMORY_SIZE) {
        std::cout << "Error: Memory Write Out of Bounds at " << address << std::endl;
        return;
    }
    memory[address] =value & 0xFF;
    memory[address+1] = (value>>8)& 0xFF;
    memory[address+2] = (value>>16) & 0xFF;
    memory[address+3] = (value>>24) & 0xFF;



}

    int32_t get_imm_s(uint32_t instruction) {
    int32_t imm_11_5 = (int32_t)(instruction &0xFE000000)>>20;
    int32_t imm_4_0 = (instruction>>7)&0x1F;
    return imm_11_5|imm_4_0;
}
    int32_t get_imm_b(uint32_t instruction) {
    int32_t bit_12 = (int32_t)(instruction&0x80000000)>>19;
    int32_t imm = 0;

    imm|= ((int32_t)instruction>>31)<<12;

    imm|= ((instruction>>7)&0x1)<<11;

    imm|= ((instruction>>25)&0x3F)<<5;

    imm|= ((instruction>>8) & 0xF)<<1;
    return imm;
}
    int32_t get_imm_j(uint32_t instruction) {
    int32_t imm =0;

    imm|= ((int32_t)instruction>>31)<<20;

    imm |= (instruction & 0xFF000) ;

    imm|= ((instruction>>20)&0x1)<<11;

    imm|= ((instruction>>21)&0x3FF)<<1;
    return imm;
}
    int32_t get_imm_u(uint32_t instruction) {
        return  (int32_t)(instruction&0xFFFFF000);
    }
};
void create_test_binary(const std::string &file_name, const std::vector<uint8_t> &data) {
    std::ofstream file(file_name, std::ios::binary);
    file.write((const char*)data.data(), data.size());
    file.close();
    std::cout << "File created: " << file_name << std::endl;
}
int main(){
CPU my_cpu;
std::string command;
std::string arg;

    Assembler as;

    std::cout << "--- RISC-V Emulator Shell (RV32IM) ---" << std::endl;
    std::cout << "Commands: load <file>, step, run, regs, reset, exit" << std::endl;

    while (true) {
        std::cout << ">> ";
        std::cin >> command;

        if (command == "exit") {
            break;
        }
        else if (command == "load") {
            std::cin >> arg;
            if (my_cpu.load_from_file(arg)) {
                my_cpu.pc =0;
                my_cpu.regs[2] =MEMORY_SIZE;
            }
        }
        else if (command == "step") {
            // Run exactly one instruction
            uint32_t inst = my_cpu.fetch();
            if (inst == 0) {
                std::cout << "End of program (Instruction 0x00)" << std::endl;
            } else {
                my_cpu.execute(inst);
            }
        }
        else if (command == "run") {
            // Run until 0x00 or max cycles
            std::cout << "Running..." << std::endl;
            int cycles = 0;
            while (cycles < 1000) { // Safety limit
                uint32_t inst = my_cpu.fetch();
                if (inst == 0) break;
                my_cpu.execute(inst);
                cycles++;
            }
            std::cout << "Stopped after " << cycles << " cycles." << std::endl;
        }
        else if (command == "regs") {
            my_cpu.dump_registers();
        }
        else if (command == "reset") {
            my_cpu.pc =0;
            std::fill(std::begin(my_cpu.regs), std::end(my_cpu.regs), 0);
            my_cpu.regs[2] = MEMORY_SIZE;
            std::cout << "CPU Reset." << std::endl;
        }
        else if (command == "asm") {
            std::cout << "Enter Assembly (Type 'END' to finish, use labels like 'loop:'):" << std::endl;
            std::string line;
            std::getline(std::cin, line);

            std::string full_source_code = "";

            while (true) {
                std::cout << "  ";
                std::getline(std::cin, line);
                if (line == "END") break;
                full_source_code += line + "\n";
            }

            std::vector<uint8_t> binary = as.compile(full_source_code, my_cpu.pc);

            std::cout << "Assembled " << binary.size() << " bytes." << std::endl;

            // uint32_t machine_code = as.assemble_line(line);
            //
            // std::cout << "Assembled: 0x" << std::hex << machine_code << std::dec << std::endl;

            for (uint8_t byte : binary) {
                if (my_cpu.pc < MEMORY_SIZE) {
                    my_cpu.memory[my_cpu.pc] = byte;
                    my_cpu.pc++;
                }
            }

            // my_cpu.memory[my_cpu.pc]     = machine_code & 0xFF;
            // my_cpu.memory[my_cpu.pc + 1] = (machine_code >> 8) & 0xFF;
            // my_cpu.memory[my_cpu.pc + 2] = (machine_code >> 16) & 0xFF;
            // my_cpu.memory[my_cpu.pc + 3] = (machine_code >> 24) & 0xFF;

            // my_cpu.pc += 4;
        }
        else {
            std::cout << "Unknown command." << std::endl;
            // Clear buffer in case of weird input
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

//     std::vector<uint8_t> sample_program = {
//         0xEF, 0x00, 0x80, 0x00, // JAL x1, 8
//         0x13, 0x00, 0x00, 0x00, // STOP (PC=4)
//         0x13, 0x01, 0x80, 0x04, // ADDI x2, x0, 72 ('H')
//         0x23, 0x28, 0x20, 0x0E, // SW x2, 240(x0)  <-- FIXED LINE
//         0x67, 0x80, 0x00, 0x00  // JALR x0, 0(x1)
//     };
// my_cpu.load_program(sample_program);
//1.Fetch
// uint32_t fetched_instruction = my_cpu.fetch();
//
// my_cpu.execute(fetched_instruction);
    // for (int i = 0; i < 6; i++) {
    //     std::cout << "--- Cycle " << i << " ---" << std::endl;
    //     uint32_t inst = my_cpu.fetch();
    //     if (inst == 0 && my_cpu.pc >= sample_program.size()) break; // Stop if end of program
    //     my_cpu.execute(inst);
    //     // my_cpu.dump_registers();
    // }



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
    // std::vector<uint8_t> fib_program = {
    //     // Init: x1=0, x2=1, x3=7, x4=0
    //     0x93, 0x00, 0x00, 0x00,
    //     0x13, 0x01, 0x10, 0x00,
    //     0x93, 0x01, 0x70, 0x00,
    //     0x13, 0x02, 0x00, 0x00,
    //
    //     // LOOP START (Offset 16)
    //     // BEQ x4, x3, +24 (Jump to END at offset 40)
    //     // FIXED: Changed 0x06 to 0x0C. (0xC << 1 = 24 bytes)
    //     0x63, 0x0C, 0x32, 0x00,
    //
    //     // ADD x5, x0, x2 (temp = next)
    //     0xB3, 0x02, 0x20, 0x00,
    //
    //     // ADD x2, x1, x2 (next = curr + next)
    //     // FIXED: Changed 0x01 to 0x81. This sets rs1 to x1 instead of x0.
    //     0x33, 0x81, 0x20, 0x00,
    //
    //     // ADD x1, x0, x5 (curr = temp)
    //     0xB3, 0x00, 0x50, 0x00,
    //
    //     // ADDI x4, x4, 1 (i++)
    //     0x13, 0x02, 0x12, 0x00,
    //
    //     // BEQ x0, x0, -20 (Loop back)
    //     0xE3, 0x06, 0x00, 0xFE,
    //
    //     // END (Offset 40)
    //     0x23, 0x28, 0x10, 0x0E, // SW x1, 240(x0)
    //     0x13, 0x00, 0x00, 0x00  // NOP
    // };
    // create_test_binary("fib.bin", fib_program);
    //
    // if (!my_cpu.load_from_file("fib.bin")) {
    //     return 1;
    // }

    // std::cout << "Starting Execution..." << std::endl;
    // my_cpu.load_program(fib_program);
    //
    // std::cout << "Calculating Fibonacci..." << std::endl;

    // Run for enough cycles to finish the loop
    // for (int i = 0; i < 50; i++) {
    //     uint32_t inst = my_cpu.fetch();
    //     if (inst == 0 && my_cpu.pc >= fib_program.size()) break;
    //     my_cpu.execute(inst);
    // }
    // std::vector<uint8_t> slt_program = {
    //     // Init: x10 = -1, x11 = 1
    //     0x13, 0x05, 0xF0, 0xFF,
    //     0x93, 0x05, 0x10, 0x00,
    //
    //     // Test 1: Signed Comparison (SLT x1, x10, x11)
    //     // FIXED: 0xB3 (Odd Dest) instead of 0x33
    //     0xB3, 0x20, 0xB5, 0x00,
    //
    //     // Test 2: Unsigned Comparison (SLTU x2, x10, x11)
    //     // 0x33 is correct (Even Dest x2)
    //     0x33, 0x31, 0xB5, 0x00,
    //
    //     // Test 3: Byte Store/Load
    //     0x13, 0x06, 0xB0, 0x0A,
    //     0x13, 0x07, 0x40, 0x06,
    //     0x23, 0x00, 0xC7, 0x00,
    //
    //     // Load back into x3
    //     // FIXED: 0x83 (Odd Dest x3) instead of 0x03
    //     0x83, 0x01, 0x07, 0x00,
    //
    //     0x13, 0x00, 0x00, 0x00
    // };
    //
    //
    // create_test_binary("slt_test.bin", slt_program);
    // my_cpu.load_from_file("slt_test.bin");
    //
    // for (int i = 0; i < 10; i++) {
    //     uint32_t inst = my_cpu.fetch();
    //     if (inst == 0) break;
    //     my_cpu.execute(inst);
    // }
    // std::vector<uint8_t> mul_program = {
    //     // Init: x11 (Result) = 1, x12 (Counter) = 5
    //     0x93, 0x05, 0x10, 0x00, // ADDI x11, x0, 1
    //     0x13, 0x06, 0x50, 0x00, // ADDI x12, x0, 5
    //
    //     // LOOP START (Offset 8)
    //     // Check if Counter (x12) is 0. If yes, jump to end (+16 bytes).
    //     0x63, 0x08, 0x06, 0x00,
    //
    //     // THE NEW INSTRUCTION: MUL x11, x11, x12
    //     // Result = Result * Counter
    //     // Note: 0xB3 start byte means it targets odd register x11.
    //     0xB3, 0x85, 0xC5, 0x02,
    //
    //     // Decrement Counter: x12 = x12 - 1
    //     0x13, 0x06, 0xF6, 0xFF,
    //
    //     // JUMP BACK (-12 bytes)
    //     0x6F, 0xF0, 0x5F, 0xFF,
    //
    //     // END
    //     0x13, 0x00, 0x00, 0x00  // NOP
    // };

    // create_test_binary("mul_test.bin", mul_program);
    // my_cpu.load_from_file("mul_test.bin");
    //
    // // Run enough cycles to finish the loop
    // for (int i = 0; i < 40; i++) {
    //     uint32_t inst = my_cpu.fetch();
    //     if (inst == 0) break;
    //     my_cpu.execute(inst);
    // }
    // my_cpu.dump_registers();

    // VERIFICATION:
    // x1 (curr) should be 13 (0xD)
    // x2 (next) should be 21 (0x15)

return 0;
}