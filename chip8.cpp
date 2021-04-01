/*
DISCLAIMER: For sources and informations I used: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM, https://austinmorlan.com/posts/chip8_emulator/#the-instructions


I copied only few snippets of the code and wrote most of it myself.
Comments are often copied from the explanations on the websites.



Author: Timon Gärtner
License: "MIT License"
*/
#include<iostream>

using namespace std;

//rand() % 255;

const unsigned int start_address = 0x200;
const unsigned int fontset_start_address = 0x50;



class Chip8{ //The whole chip, it's architecture and all it's functions (load memory etc.)
public:
    uint8_t registers[16]; //16 8-bit Registers
    uint8_t memory[4096]; //4K Bytes of Memory 
    uint16_t index_register; //16-bit Index Register -> store memory addresses
    uint16_t pc; // 16-bit Program Counter -> address of next instruction to execute
    uint16_t stack; //16-level Stack -> keep track of the order of execution = 16 different PCs
    uint8_t sp; //Stackpointer -> tells us, where our recent value in the level_stack was placed
    uint8_t delay_timer; // for timing -> if it's > 0 it'll decrement at a rate of 60Hz
    uint8_t sound_timer; // timer for sound, if it's not zero a single tone will play, it works the same as the delay timer
    uint8_t keys[16]; //keypad (4*4)
    uint32_t display_memory[2048]; //display, 0xFFFFFFFF=on, 0x00000000=off


    uint8_t fontset[80] = {
	                        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	                        0x20, 0x60, 0x20, 0x20, 0x70, // 1
	                        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	                        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	                        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	                        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	                        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	                        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	                        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	                        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	                        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	                        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    	                    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	                        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	                        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	                        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
                            };

    
    void start(){ //called at the "boot" of the chip
        pc = start_address; //resets the pc to the right address
        load_rom("");
        load_fontset(); //loads fontset into memory
    }

    void load_rom(string filename) {//loads instructions of the rom into the memory

    }

    void set_memory(unsigned int address, uint8_t content){ 

    }

    void set_display_memory(unsigned int address, uint8_t content){ 
        display_memory[address] = content;
    }

    
    void load_fontset() {

    }

    


//
//
    //Chip8 OPCODES
//
//
/*void op_0nnn(){ //Jump to a machine code routine at nnn
This instruction is only used on the old computers on which Chip-8 was originally implemented. It is ignored by modern interpreters.
}*/
void op_00E0(){ //CLS - clears display          check
    for(int i = 0; i<2048; i++){
        set_display_memory(i, 0);
    }
}
void op_00EE(){ //RET - return from a subroutine            

}

void op_1nnn(){ //JP addr - jump to location nnn 

}
void op_2nnn(){ //Call addr - call subroutine at nnn

}
void op_3xkk(){ //SE Vx, byte - Skips next instruction if Vx == kk

}
void op_4xkk(){ //Skip next instruction if Vx != kk

}
void op_5xy0(){ // Skip next instruction if Vx == Vy.

}
void op_6xkk(){ // Set Vx = kk.

}
void op_7xkk(){ // Set Vx = Vx + kk.

}
void op_8xy0(){ // Set Vx = Vy.

}
void op_8xy1(){ // Set Vx = Vx OR Vy

}
void op_8xy2(){ // Set Vx = Vx AND Vy

}
void op_8xy3(){ // Set Vx = Vx XOR Vy

}
void op_8xy4(){ // Set Vx = Vx + Vy, set VF = carry

}
void op_8xy5(){ // Set Vx = Vx - Vy, set VF = NOT borrow

}
void op_8xy6(){ // Set Vx = Vx SHR 1

}
void op_8xy7(){ //Set Vx = Vy - Vx, set VF = NOT borrow

}
void op_8xyE(){ // Set Vx = Vx SHL 1

}
void op_9xy0(){ // Skip next instruction if Vx != Vy

}
void op_Annn(){ // Set I = nnn

}
void op_Bnnn(){  //Jump to location nnn + V0

}
void op_Cxkk(){ // Set Vx = random byte AND kk

}
void op_Dxyn(){ // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision

}
void op_Ex9e(){ // Skip next instruction if key with the value of Vx is pressed

}
void op_ExA1(){ // Skip next instruction if key with the value of Vx is not pressed

}
void op_Fx07(){ // Set Vx = delay timer value

}
void op_Fx0A(){ // Wait for a key press, store the value of the key in Vx

}
void op_Fx15(){ // Set delay timer = Vx

}
void op_Fx18(){  //Set sound timer = Vx

}
void op_Fx1E(){ // Set I = I + Vx

}
void op_Fx29(){ // Set I = location of sprite for digit Vx

}
void op_Fx33(){ // Store BCD representation of Vx in memory locations I, I+1, and I+2

}
void op_Fx55(){ // Store registers V0 through Vx in memory starting at location I

}
void op_Fx65(){ // Read registers V0 through Vx from memory starting at location I

}


};





int main() { 
    Chip8 chip8;
    
    chip8.start(); //called at the "boot" of the chip
    chip8.op_00E0();
    
    
    return 0;
}





