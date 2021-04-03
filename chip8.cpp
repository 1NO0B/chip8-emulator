/*
DISCLAIMER: For sources and informations I used: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM, https://austinmorlan.com/posts/chip8_emulator/#the-instructions


I copied only few snippets of the code and wrote most of it myself.
Comments are often copied from the explanations on the websites.



Author: Timon Gärtner
License: "MIT License"
*/
#define SDL_MAIN_HANDLED
#include<iostream>
#include<cstdint>
#include<fstream>
#include<chrono>
#include<SDL2/SDL.h>


using namespace std;

//rand() % 255;

char const* filename = "PONG";

const unsigned int start_address = 0x200;
const unsigned int fontset_start_address = 0x50;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;





class Chip8{ //The whole chip, it's architecture and all it's functions (load memory etc.)
public:
    uint8_t registers[16]; //16 8-bit Registers
    uint8_t memory[4096]; //4K Bytes of Memory 
    uint16_t index_register; //16-bit Index Register -> store memory addresses
    uint16_t pc; // 16-bit Program Counter -> address of next instruction to execute
    uint16_t stack[16]; //16-level Stack -> keep track of the order of execution = 16 different PCs
    uint8_t sp; //Stackpointer -> tells us, where our recent value in the level_stack was placed
    uint8_t delay_timer; // for timing -> if it's > 0 it'll decrement at a rate of 60Hz
    uint8_t sound_timer; // timer for sound, if it's not zero a single tone will play, it works the same as the delay timer
    uint8_t keys[16]; //keypad (4*4)
    uint32_t display_memory[2048]{}; //display, 0xFFFFFFFF=on, 0x00000000=off
    uint16_t opcode;


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

    
    

    void load_rom() {//loads instructions of the rom into the memory
        FILE* rom = fopen(filename, "rb");
    if (rom == NULL) {
        std::cerr << "Failed to open ROM" << std::endl;
        return;
    }

    // Get file size
    fseek(rom, 0, SEEK_END);
    long rom_size = ftell(rom);
    rewind(rom);

    // Allocate memory to store rom
    char* rom_buffer = (char*) malloc(sizeof(char) * rom_size);
    if (rom_buffer == NULL) {
        std::cerr << "Failed to allocate memory for ROM" << std::endl;
        return;
    }

    // Copy ROM into buffer
    size_t result = fread(rom_buffer, sizeof(char), (size_t)rom_size, rom);
    if (result != rom_size) {
        std::cerr << "Failed to read ROM" << std::endl;
        return;
    }

    // Copy buffer to memory
    if ((4096-512) > rom_size){
        for (int i = 0; i < rom_size; ++i) {
            memory[i + 512] = (uint8_t)rom_buffer[i];   // Load into memory starting
                                                        // at 0x200 (=512)
        }
    }
    else {
        std::cerr << "ROM too large to fit in memory" << std::endl;
        return;
    }

    // Clean up
    fclose(rom);
    free(rom_buffer);

    }


    
    void load_fontset() {
        for(int i = 0; i < 80; i++){
            memory[fontset_start_address + i] = fontset[i];
        }
    }

    void start(){ //called at the "boot" of the chip

        
        pc = start_address; //resets the pc to the right address
        load_rom();
        load_fontset(); //loads fontset into memory
        op_00E0();
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
        display_memory[i]=0; //sets all values of display_memory[] to 0
    }
}
void op_00EE(){ //RET - return from a subroutine            
    sp--;
    pc = stack[sp];
}

void op_1nnn(){ //JP addr - jump to location nnn 
    pc = opcode & 0x0FFF;
}
void op_2nnn(){ //Call addr - call subroutine at nnn
    stack[sp] = pc; 
    sp++;
    pc = opcode & 0x0FFF;
}
void op_3xkk(){ //SE Vx, byte - Skips next instruction if Vx == kk
    if(registers[(opcode & 0x0F00)>>8] == (opcode & 0x00FF)){
        pc+=2;
    }
}
void op_4xkk(){ //Skip next instruction if Vx != kk
    if(registers[(opcode & 0x0F00)>>8] != (opcode & 0x00FF)){
        pc+=2;
    }
}
void op_5xy0(){ // Skip next instruction if Vx == Vy.
    if(registers[(opcode & 0x0F00)>>8] == registers[(opcode & 0x00F0)>>4]){
        pc+=2;
    }
}
void op_6xkk(){ // Set Vx = kk.
    registers[(opcode & 0x0F00)>>8] = opcode & 0x00FF;
}
void op_7xkk(){ // Set Vx = Vx + kk.
    registers[(opcode & 0x0F00)>>8] += opcode & 0x00FF;
}
void op_8xy0(){ // Set Vx = Vy.
    registers[(opcode & 0x0F00)>>8]=registers[(opcode & 0x00F0)>>4];
}
void op_8xy1(){ // Set Vx = Vx OR Vy
    registers[(opcode & 0x0F00)>>8] |= registers[(opcode & 0x00F0)>>4];
}
void op_8xy2(){ // Set Vx = Vx AND Vy
    registers[(opcode & 0x0F00)>>8] &= registers[(opcode & 0x00F0)>>4];
}
void op_8xy3(){ // Set Vx = Vx XOR Vy
    registers[(opcode & 0x0F00)>>8] ^= registers[(opcode & 0x00F0)>>4];
}
void op_8xy4(){ // Set Vx = Vx + Vy, set VF = carry
    registers[(opcode & 0x0F00)>>8] += registers[(opcode & 0x00F0)>>4];

    if(registers[(opcode & 0x0F00)>>8] > 255){
        registers[0xF]=1;
    }
    else{
        registers[0xF]=0;
    }
}
void op_8xy5(){ // Set Vx = Vx - Vy, set VF = NOT borrow
    if(registers[(opcode & 0x0F00)>>8] > registers[(opcode & 0x00F0)>>4]){
        registers[0xF]=1;
    }
    else{
        registers[0xF]=0;
    }
    registers[(opcode & 0x0F00)>>8] -= registers[(opcode & 0x00F0)>>4];
    
}
void op_8xy6(){ // Set Vx = Vx SHR 1
    registers[0xF] = registers[(opcode & 0x0F00) >> 8] & 0x1;
    registers[(opcode & 0x0F00) >> 8] >>= 1;
}
void op_8xy7(){ //Set Vx = Vy - Vx, set VF = NOT borrow
    if(registers[(opcode & 0x00F0)>>4] > registers[(opcode & 0x0F00) >> 8]){
        registers[0xF]=1;
    }
    else{
        registers[0xF]=0;
    }
    registers[(opcode & 0x0F00) >> 8] = registers[(opcode & 0x00F0)>>4] - registers[(opcode & 0x0F00) >> 8];
}
void op_8xyE(){ // Set Vx = Vx SHL 1
    registers[0xF] = registers[(opcode & 0x0F00) >> 8] >> 7;
    registers[(opcode & 0x0F00) >> 8] <<= 1;
}
void op_9xy0(){ // Skip next instruction if Vx != Vy
    if(registers[(opcode & 0x0F00)>>8] != registers[(opcode & 0x00F0)>>4]){
        pc+=2;
    }
}
void op_Annn(){ // Set I = nnn
    index_register = opcode & 0x0FFF;
}
void op_Bnnn(){  //Jump to location nnn + V0
    pc = (opcode & 0x0FFF) + registers[0];
}
void op_Cxkk(){ // Set Vx = random byte AND kk
    registers[(opcode & 0x0F00)>>8] = rand() % 255 & (opcode & 0x00FF);
}
void op_Dxyn(){ // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
    unsigned short x = registers[(opcode & 0x0F00) >> 8];
            unsigned short y = registers[(opcode & 0x00F0) >> 4];
            unsigned short height = opcode & 0x000F;
            unsigned short pixel;

            registers[0xF] = 0;
            for (int yline = 0; yline < height; yline++)
            {
                pixel = memory[index_register + yline];
                for(int xline = 0; xline < 8; xline++)
                {
                    if((pixel & (0x80 >> xline)) != 0)
                    {
                        if(display_memory[(x + xline + ((y + yline) * 64))] == 1)
                        {
                            registers[0xF] = 1;
                        }
                        display_memory[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }
    
}

void op_Ex9e(){ // Skip next instruction if key with the value of Vx is pressed
    if(keys[registers[(opcode & 0x0F00)>>8]] == true){
        pc+=2;
    }
}
void op_ExA1(){ // Skip next instruction if key with the value of Vx is not pressed
    if(keys[registers[(opcode & 0x0F00)>>8]] == false){
        pc+=2;
    }
}
void op_Fx07(){ // Set Vx = delay timer value
    registers[(opcode & 0x0F00)>>8] = delay_timer;
}
void op_Fx0A(){ // Wait for a key press, store the value of the key in Vx
    bool keypressed = false;
    for (int i = 0; i<15; i++){   
        if(keys[i] == true){
            registers[(opcode & 0x0F00)>>8] = keys[i];
            keypressed = true;
        }
    }
    if(keypressed==false){
        pc -= 2;
    }
}
void op_Fx15(){ // Set delay timer = Vx
    delay_timer = registers[(opcode & 0x0F00)>>8];
}
void op_Fx18(){  //Set sound timer = Vx
    sound_timer = registers[(opcode & 0x0F00)>>8];
}
void op_Fx1E(){ // Set I = I + Vx
    index_register += registers[(opcode & 0x0F00)>>8];
}
void op_Fx29(){ // Set I = location of sprite for digit Vx
    index_register = fontset_start_address + (5*registers[(opcode & 0x0F00)>>8]);
}
void op_Fx33(){ // Store BCD representation of Vx in memory locations I, I+1, and I+2
    uint8_t value = registers[(opcode & 0x0F00)>>8];
    memory[index_register+2] = value % 10;
    value/=10;
    memory[index_register+1] = value % 10;
    value/=10;
    memory[index_register] = value % 10;
    
}
void op_Fx55(){ // Store registers V0 through Vx in memory starting at location I
    for(int i = 0; i <= registers[(opcode & 0x0F00)>>8]; i++){
        memory[index_register+i] = registers[i];
    }
}
void op_Fx65(){ // Read registers V0 through Vx from memory starting at location I
    for(int i = 0; i <= registers[(opcode & 0x0F00)>>8]; i++){
         registers[i] = memory[index_register+i];
    }
}

void opcode_switch(){ //switch with all the opcodes
    switch(opcode & 0xF000){
        case 0x0000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    op_00E0();
                    break;
                case 0x000E:
                    op_00EE();
                    break;
            }
            break;
        case 0x1000:
            op_1nnn();
            break;
        case 0x2000:
            op_2nnn();
            break;
        case 0x3000:
            op_3xkk();
            break;
        case 0x4000:
            op_4xkk();
            break;
        case 0x5000:
            op_5xy0();
            break;
        case 0x6000:
            op_6xkk();
            break;
        case 0x7000:
            op_7xkk();
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    op_8xy0();
                    break;
                case 0x0001:
                    op_8xy1();
                    break;
                case 0x0002:
                    op_8xy2();
                    break;
                case 0x0003:
                    op_8xy3();
                    break;
                case 0x0004:
                    op_8xy4();
                    break;
                case 0x0005:
                    op_8xy5();
                    break;
                case 0x0006:
                    op_8xy6();
                    break;
                case 0x0007:
                    op_8xy7();
                    break;
                case 0x000E:
                    op_8xyE();
                    break;
            }
            break;
        case 0x9000:
            op_9xy0();
            break;
        case 0xA000:
            op_Annn();
            break;
        case 0xB000:
            op_Bnnn();
            break;
        case 0xC000:
            op_Cxkk();
            break;
        case 0xD000:
            op_Dxyn();
            break;
        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E:
                    op_Ex9e();
                    break;
                case 0x00A1:
                    op_ExA1();
                    break;
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF){
                case 0x0007:
                    op_Fx07();
                    break;
                case 0x000A:
                    op_Fx0A();
                    break;
                case 0x0015:
                    op_Fx15();
                    break;
                case 0x0018:
                    op_Fx18();
                    break;
                case 0x001E:
                    op_Fx1E();
                    break;
                case 0x0029:
                    op_Fx29();
                    break;
                case 0x0033:
                    op_Fx33();
                    break;
                case 0x0055:
                    op_Fx55();
                    break;
                case 0x0065:
                    op_Fx65();
                    break;
            }
            break;
    }
}

void cycle(){ //execudes one cycle of the cpu

    //sets the opcode to the fetched value of this memory-address and the next
    opcode = (memory[pc]<<8) | (memory[pc+1]);

    //increases pc to the next step
    pc+=2;


    //decode the opcode and execute the proper function 
    opcode_switch();

    //decrement delay timer if it's > 0
    if(delay_timer>0){
        delay_timer--;
    }

    //decrement sound timer if it's > 0
    if(sound_timer>0){
        sound_timer--;
    }
}

};


class SDL_Window;
class SDL_Renderer;
class SDL_Texture;


class Platform
{
public:
	Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight);
	~Platform();
	void Update(void const* buffer, int pitch);
	bool ProcessInput(uint8_t* keys);

private:
	SDL_Window* window{};
	SDL_Renderer* renderer{};
	SDL_Texture* texture{};
};
Platform::Platform(char const* title, int windowWidth, int windowHeight, int textureWidth, int textureHeight)
{
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(title, 0, 0, windowWidth, windowHeight, SDL_WINDOW_SHOWN);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, textureWidth, textureHeight);
}

Platform::~Platform()
{
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Platform::Update(void const* buffer, int pitch)
{
	SDL_UpdateTexture(texture, nullptr, buffer, pitch);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(renderer);
}

bool Platform::ProcessInput(uint8_t* keys)
{
	bool quit = false;

	SDL_Event event;

	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
			{
				quit = true;
			} break;

			case SDL_KEYDOWN:
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
					{
						quit = true;
					} break;

					case SDLK_x:
					{
						keys[0] = 1;
					} break;

					case SDLK_1:
					{
						keys[1] = 1;
					} break;

					case SDLK_2:
					{
						keys[2] = 1;
					} break;

					case SDLK_3:
					{
						keys[3] = 1;
					} break;

					case SDLK_q:
					{
						keys[4] = 1;
					} break;

					case SDLK_w:
					{
						keys[5] = 1;
					} break;

					case SDLK_e:
					{
						keys[6] = 1;
					} break;

					case SDLK_a:
					{
						keys[7] = 1;
					} break;

					case SDLK_s:
					{
						keys[8] = 1;
					} break;

					case SDLK_d:
					{
						keys[9] = 1;
					} break;

					case SDLK_z:
					{
						keys[0xA] = 1;
					} break;

					case SDLK_c:
					{
						keys[0xB] = 1;
					} break;

					case SDLK_4:
					{
						keys[0xC] = 1;
					} break;

					case SDLK_r:
					{
						keys[0xD] = 1;
					} break;

					case SDLK_f:
					{
						keys[0xE] = 1;
					} break;

					case SDLK_v:
					{
						keys[0xF] = 1;
					} break;
				}
			} break;

			case SDL_KEYUP:
			{
				switch (event.key.keysym.sym)
				{
					case SDLK_x:
					{
						keys[0] = 0;
					} break;

					case SDLK_1:
					{
						keys[1] = 0;
					} break;

					case SDLK_2:
					{
						keys[2] = 0;
					} break;

					case SDLK_3:
					{
						keys[3] = 0;
					} break;

					case SDLK_q:
					{
						keys[4] = 0;
					} break;

					case SDLK_w:
					{
						keys[5] = 0;
					} break;

					case SDLK_e:
					{
						keys[6] = 0;
					} break;

					case SDLK_a:
					{
						keys[7] = 0;
					} break;

					case SDLK_s:
					{
						keys[8] = 0;
					} break;

					case SDLK_d:
					{
						keys[9] = 0;
					} break;

					case SDLK_z:
					{
						keys[0xA] = 0;
					} break;

					case SDLK_c:
					{
						keys[0xB] = 0;
					} break;

					case SDLK_4:
					{
						keys[0xC] = 0;
					} break;

					case SDLK_r:
					{
						keys[0xD] = 0;
					} break;

					case SDLK_f:
					{
						keys[0xE] = 0;
					} break;

					case SDLK_v:
					{
						keys[0xF] = 0;
					} break;
				}
			} break;
		}
	}

	return quit;
}


int main(int argc, char** argv)
{
	

	int videoScale = 20;
	int cycleDelay = 1;
	filename = "PONG";

	Platform platform("CHIP-8 Emulator", VIDEO_WIDTH * videoScale, VIDEO_HEIGHT * videoScale, VIDEO_WIDTH, VIDEO_HEIGHT);

	Chip8 chip8;
	chip8.start();

	int videoPitch = sizeof(chip8.display_memory[0]) * VIDEO_WIDTH;

	//auto lastCycleTime = chrono::high_resolution_clock::now();
	bool quit = false;

	while (!quit)
	{
		quit = platform.ProcessInput(chip8.keys);

		//auto currentTime = chrono::high_resolution_clock::now();
		//float dt = chrono::duration<float, std::chrono::milliseconds::period>(currentTime - lastCycleTime).count();

		if (true)
		{
			//lastCycleTime = currentTime;

			chip8.cycle();

			platform.Update(chip8.display_memory, videoPitch);
		}
	}

	return 0;
}
