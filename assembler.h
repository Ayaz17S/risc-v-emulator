#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>

class Assembler {
private:
    std::map<std::string, uint32_t> symbol_table;

    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (std::string::npos == first) return str;
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, last - first + 1);
    }

    uint32_t parse_reg(std::string reg) {
        reg = trim(reg);
        if (reg.empty()) return 0;
        if (reg[0]== 'x'||reg[0]=='X') return std::stoi(reg.substr(1));
        return 0;
    }

public:

    std::vector<uint8_t> compile(std::string source_code, uint32_t start_pc) {
        std::vector<std::string> lines;
        std::stringstream ss(source_code);
        std::string line;

        uint32_t current_pc = start_pc;
        while (std::getline(ss, line)) {
            line = trim(line);
            if (line.empty()||line[0]=='#') continue;

            if (line.back() == ':') {
                std::string label = line.substr(0, line.size()-1);
                symbol_table[label] = current_pc;
                continue;
            }
            lines.push_back(line);
            current_pc+=4;

        }

        std::vector<uint8_t> binary;
        current_pc = start_pc;
        for (const auto& instr: lines) {
            std::stringstream ls(instr);
            std::string mnemonic,arg1,arg2,arg3;
            ls >> mnemonic;
            std::getline(ls,arg1,',');
            std::getline(ls,arg2,',');
            std::getline(ls,arg3);

            uint32_t machine_code = 0;
            uint32_t rd = parse_reg(arg1);
            uint32_t rs1 = parse_reg(arg2);

            if (mnemonic == "ADDI") {
                int32_t imm = std::stoi(trim(arg3));
                machine_code = (imm<<20)|(rs1<<15)| (0x0<<12)|(rd<<7) | 0x13;
            }
            else if(mnemonic =="ADD") {
                uint32_t rs2 = parse_reg(arg3);
                machine_code = (0x00 << 25) | (rs2 << 20) | (rs1 << 15) | (0x0 << 12) | (rd << 7) | 0x33;
            }
            else if (mnemonic == "MUL") {
                uint32_t rs2 = parse_reg(arg3);
                machine_code = (0x01 << 25) | (rs2 << 20) | (rs1 << 15) | (0x0 << 12) | (rd << 7) | 0x33;
            }
            else if (mnemonic == "BEQ") {
                std::string label = trim(arg3);
                uint32_t target = symbol_table[label];
                int32_t offset = target - current_pc;

                uint32_t imm12 = (offset<<12) &1;
                uint32_t imm10_5 = (offset<<5) &0x3F;
                uint32_t imm4_1 = (offset >> 1) & 0xF;
                uint32_t imm11 = (offset >> 11) & 1;

                uint32_t rs2 = parse_reg(arg1);
                rs1 = parse_reg(arg1);
                uint32_t rs2_real = parse_reg(arg2);

                machine_code = (imm12 << 31) | (imm10_5 << 25) | (rs2_real << 20) | (rs1 << 15) | (0x0 << 12) | (imm4_1 << 8) | (imm11 << 7) | 0x63;
            }

            binary.push_back(machine_code&0xFF);
            binary.push_back((machine_code >> 8) & 0xFF);
            binary.push_back((machine_code >> 16) & 0xFF);
            binary.push_back((machine_code >> 24) & 0xFF);

            current_pc+=4;
        }
        return binary;
    }
    };
    // std::vector<std::string> tokenize( std::string& line) {
    //     std::vector<std::string> tokens;
    //     std::string token;
    //     for (char c: line) {
    //         if (c == ',' || c == ' ') {
    //             if (!token.empty()) {
    //                 tokens.push_back(token);
    //                 token = "";
    //             }
    //         } else{
    //             token += c;
    //         }
    //     }
    //     if (!token.empty()) tokens.push_back(token);
    //     return tokens;
    // }
    // uint32_t parse_reg(std::string reg_name) {
    //     if (reg_name[0]=='x') {
    //         return std::stoi(reg_name.substr(1));
    //     }
    //     return 0;
    // }
    // uint32_t assemble_line(std::string line) {
    //     std::vector<std::string> tokens = tokenize(line);
    //     if (tokens.empty())return 0;
    //
    //     std::string mnemonic = tokens[0];
    //
    //     if (mnemonic == "ADDI") {
    //         uint32_t rd = parse_reg(tokens[1]);
    //         uint32_t rs1 = parse_reg(tokens[2]);
    //         int32_t imm = std::stoi(tokens[3]);
    //
    //         uint32_t opcode = 0x13;
    //         uint32_t funct3 = 0x0;
    //
    //         return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    //     }
    //     else if (mnemonic == "ADD") {
    //         uint32_t rd  = parse_reg(tokens[1]);
    //         uint32_t rs1 = parse_reg(tokens[2]);
    //         uint32_t rs2 = parse_reg(tokens[3]);
    //
    //         uint32_t opcode = 0x33;
    //         uint32_t funct3 = 0x0;
    //         uint32_t funct7 = 0x00;
    //
    //         return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    //     }
    //
    //     else if (mnemonic == "MUL") {
    //         uint32_t rd  = parse_reg(tokens[1]);
    //         uint32_t rs1 = parse_reg(tokens[2]);
    //         uint32_t rs2 = parse_reg(tokens[3]);
    //
    //         uint32_t opcode = 0x33;
    //         uint32_t funct3 = 0x0;
    //         uint32_t funct7 = 0x01;
    //
    //         return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
    //     }
    //     std::cout << "Unknown instruction: " << mnemonic << std::endl;
    //     return 0;
    // }
// };
#endif
