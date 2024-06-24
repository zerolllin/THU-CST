#include <iostream>
#include <fstream>
#include <sstream>

int memory[128][32]; 
int pdbr = 0x220;

void init_memory_from_file(std::string file_path) {
    std::ifstream file(file_path);
    
    std::string line;
    int i = 0;
    while (std::getline(file, line)) {
        int j = 0;
        std::istringstream iss(line);
        std::string page_str, hex_str;
        iss >> page_str >> hex_str;
        while (iss >> hex_str) {
            int value = std::stoi(hex_str, nullptr, 16);
            memory[i][j++] = value;
        }
        i++;
    }
}

int read_page_data(uint8_t page, uint8_t offset) {
    return memory[page][offset];
}


class PDE {
private:
    int data;
public:
    PDE(int data_): data(data_) {}
    bool is_valid() {
        return (data & 0x80) != 0;
    }
    int get_pfn() {
        return (data & 0x7f);
    }
};

class PTE {
private:
    int data;
public:
    PTE(int data_): data(data_) {}
    bool is_valid() {
        return (data & 0x80) != 0;
    }
    int get_pd() {
        return (data & 0x7f);
    }
};

class VirtualAddress {
private:
    int addr;
public:
    VirtualAddress(int data): addr(data) {}
    int get_pden() {
        return (addr & 0x7c00) >> 10;
    }
    int get_pten() {
        return (addr & 0x3e0) >> 5;
    }
    int get_offset() {
        return addr & 0x1f;
    }
};

std::string tab = "  ";

void translate(int addr) {
    VirtualAddress va(addr);
    std::cout << "Vritual Address " << std::hex << addr << "\n";
    PDE pde(read_page_data(pdbr/32, va.get_pden()));
    std::cout << tab << "--> pde index:0x" << std::hex << va.get_pden() <<"  pde contents:(valid " << (pde.is_valid() ? 1 : 0) << ", pfn 0x"<< std::hex << pde.get_pfn() << ")\n";
    if (!pde.is_valid()) {
        std::cout << tab << tab << "--> Fault (page directory entry not valid)\n";
        return;
    }
    PTE pte(read_page_data(pde.get_pfn(), va.get_pten()));
    std::cout << tab << tab << "--> pte index:0x" << std::hex << va.get_pten() <<"  pde contents:(valid " << (pte.is_valid() ? 1 : 0) << ", pd 0x"<< std::hex << pte.get_pd() << ")\n";
    if (!pte.is_valid()) {
        std::cout << tab << tab << tab << "--> Fault (page table entry not valid)\n";
        return;
    }
    std::cout << tab << tab << tab << "--> Translates to Physical Address 0x" << std::hex << va.get_offset() + 32 * pte.get_pd() << "  ";
    std::cout << "--> Value: " << std::hex << read_page_data(pte.get_pd(), va.get_offset()) << "\n";
}

int main() {
    init_memory_from_file("memory.txt");
    translate(0x6c74);
    translate(0x6b22);
    translate(0x03df);
    translate(0x69dc);
    translate(0x317a);
    translate(0x4546);
    translate(0x2c03);
    translate(0x7fd7);
    translate(0x390e);
    translate(0x748b);
}

