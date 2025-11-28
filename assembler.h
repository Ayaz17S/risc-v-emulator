#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>

class Assembler {
public:
    std::vector<std::string> tokenize( std::string& line) {
        std::vector<std::string> tokens;
        std::string token;
        for (char c: line) {
            if (c == ',' || c == ' ') {
                if (!token.empty()) {
                    tokens.push_back(token);
                    token = "";
                }
            } else{
                token += c;
            }
        }
        if (!token.empty()) tokens.push_back(token);
        return tokens;
    }
    uint32_t parse_reg(std::string reg_name) {
        if (reg_name[0]=='x') {
            return std::stoi(reg_name.substr(1));
        }
        return 0;
    }
    uint32_t assemble_line(std::string line) {
        std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty())return 0;

        std::string mnemonic = tokens[0];

        if (mnemonic == "ADDI") {
            uint32_t rd = parse_reg(tokens[1]);
            uint32_t rs1 = parse_reg(tokens[2]);
            int32_t imm = std::stoi(tokens[3]);

            uint32_t opcode = 0x13;
            uint32_t funct3 = 0x0;

            return (imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
        }
        else if (mnemonic == "ADD") {
            uint32_t rd  = parse_reg(tokens[1]);
            uint32_t rs1 = parse_reg(tokens[2]);
            uint32_t rs2 = parse_reg(tokens[3]);

            uint32_t opcode = 0x33;
            uint32_t funct3 = 0x0;
            uint32_t funct7 = 0x00;

            return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
        }

        else if (mnemonic == "MUL") {
            uint32_t rd  = parse_reg(tokens[1]);
            uint32_t rs1 = parse_reg(tokens[2]);
            uint32_t rs2 = parse_reg(tokens[3]);

            uint32_t opcode = 0x33;
            uint32_t funct3 = 0x0;
            uint32_t funct7 = 0x01;

            return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
        }
        std::cout << "Unknown instruction: " << mnemonic << std::endl;
        return 0;
    }
};
#endif
