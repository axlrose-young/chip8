#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <time.h>
#include <unistd.h>
#define DEBUG_MODE 1

typedef struct{
	uint8_t memory[4096];
	uint8_t V[16];
	uint16_t I;
	uint16_t pc;
	bool isrunning;
	int display[64*32];
	uint8_t sp;
	uint16_t stack[16];
	uint8_t keys[16];
	uint8_t delay_timer;
	uint8_t sound_timer;
}chip8;

uint8_t chip8_fontset[80] = {
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

SDL_Window* window = NULL;
SDL_Renderer* render = NULL;
int draw_flag = 0;

void sdl_event(chip8* mychip8);
void load_rom(chip8* mychip8);
void emulate_cycle(chip8* mychip8);
void render_engine(chip8* mychip8);
void disassembler(uint16_t opcode, chip8* mychip8);

int main(){
	chip8 mychip8;
	//loading rom
	
	window = SDL_CreateWindow(
        "CHIP-8 Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	640, 
	320,SDL_WINDOW_SHOWN);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	load_rom(&mychip8);
	mychip8.isrunning = true;
	mychip8.pc = 0x200;
	mychip8.sp = 0;
	memset(mychip8.keys, 0, sizeof(mychip8.keys));
	memset(mychip8.display, 0, sizeof(mychip8.display));
	srand(time(NULL));
	
	// 600Hz cpu speed - executes instructions at this speed
	// 60 Hz timer - 60 ticks in a sec
	// for instructions per tick - (600 Hz/s) / (60 ticks/s)
	// 10 instructions per tick
		
	while(mychip8.isrunning){
		for(uint8_t i=0; i<10; i++){
			sdl_event(&mychip8);
			emulate_cycle(&mychip8);
		}
		if(mychip8.delay_timer > 0){
			mychip8.delay_timer--;
		}
		if(mychip8.sound_timer > 0){
			mychip8.sound_timer--;	
		}
		if(draw_flag){
			render_engine(&mychip8);
			draw_flag = 0;
		}
		usleep(16666);
	}
	SDL_DestroyRenderer(render);
    	SDL_DestroyWindow(window);
    	SDL_Quit();
	return 0;
}

void load_rom(chip8* mychip8){
	FILE* pfile = fopen("Pong.ch8","rb");
	if(pfile == NULL){
		perror("Error opening rom\n");
	}
	memcpy(&mychip8->memory[0], chip8_fontset, 80);
	fread(&mychip8->memory[0x200],1,sizeof(mychip8->memory),pfile);
}

void emulate_cycle(chip8* mychip8){
	uint16_t opcode;
	opcode = (mychip8->memory[mychip8->pc] << 8) | mychip8->memory[mychip8->pc+1];
	// Call disassembler
	if(DEBUG_MODE){
		disassembler(opcode, mychip8);
      	}

	if(mychip8->pc >= 4096){
		mychip8->isrunning = false;
	}
	mychip8->pc+=2;

	switch(opcode&0xF000){
		case(0x0000):
			switch(opcode&0x00FF){
				case(0x00E0):
					memset(mychip8->display,0,sizeof(mychip8->display));
					draw_flag = 1;
					break;
				case(0x00EE):
					mychip8->sp--;
					mychip8->pc = mychip8->stack[mychip8->sp];
					break;	
			}
			break;
		case(0x2000):
			mychip8->stack[mychip8->sp] = mychip8->pc;	
			mychip8->sp++;
			mychip8->pc = opcode&0x0FFF;
			break;	
		case(0x4000):
			if(mychip8->V[(opcode&0x0F00)>>8]!=(opcode&0x00FF)){
				mychip8->pc+=2;
			}
			break;
		case(0x5000):
			if(mychip8->V[(opcode&0x0F00)>>8] == mychip8->V[(opcode&0x00F0)>>4]){
				mychip8->pc+=2;
			}
			break;
		case(0xA000):
			mychip8->I = opcode & 0x0FFF;
			break;
		case(0x6000):
			mychip8->V[(opcode&0x0F00)>>8]= opcode&0x00FF;
			break;
		case(0x1000):
			mychip8->pc = opcode&0x0FFF;
			break;
		case(0x7000):
			mychip8->V[(opcode&0x0F00)>>8]+=(opcode&0x00FF);
			break;
		case(0x8000):
			switch(opcode&0x000F){
				case(0x0001):
					mychip8->V[(opcode&0x0F00)>>8] |= mychip8->V[(opcode&0x00F0)>>4];
					break;
				case(0x0002):
					mychip8->V[(opcode&0x0F00)>>8] &= mychip8->V[(opcode&0x00F0)>>4];
					break;
				case(0x0003):
					mychip8->V[(opcode&0x0F00)>>8] &= mychip8->V[(opcode&0x00F0)>>4];
					break;	
				case(0x0004):
					uint16_t result = mychip8->V[(opcode&0x0F00)>>8] + mychip8->V[(opcode&0x00F0)>>4];
					if(result>255){
						mychip8->V[0xF] = 1;
					}
					else{
						mychip8->V[0xF] = 0;
					}
					mychip8->V[(opcode&0x0F00)>>8] = result & 0x00FF;
					break;	
				case(0x0005):
					if(mychip8->V[(opcode&0x0F00)>>8]>mychip8->V[(opcode&0x00F0)>>4]){
						mychip8->V[0xF] = 1;
					}
					else{
						mychip8->V[0xF] = 0;
					}
					mychip8->V[(opcode&0x0F00)>>8] -= mychip8->V[(opcode&0x00F0)>>4];
					break;	
				case(0x0006):
					if(mychip8->V[(opcode&0x0F00)>>8] & 0x01 == 1){
						mychip8->V[0xF] = 1;
					}	
					else{
						mychip8->V[0xF] = 0;
					}	
					mychip8->V[(opcode&0x0F00)>>8]>>1;
					break;
				case(0x0007):
					if(mychip8->V[(opcode&0x0F00)>>8]<mychip8->V[(opcode&0x00F0)>>4]){
						mychip8->V[0xF] = 1;
					}
					else{
						mychip8->V[0xF] = 0;
					}
					mychip8->V[(opcode&0x0F00)>>8] = mychip8->V[(opcode&0x00F0)>>4] - mychip8->V[(opcode&0xF000)>>8];
					break;
				case(0x000E):
					if(mychip8->V[(opcode&0x0F00)>>8] & 0x80 == 0x80){
						mychip8->V[0xF] = 1;
					}	
					else{
						mychip8->V[0xF] = 0;
					}
					mychip8->V[(opcode&0x0F00)>>8] << 1;
					break;
			}
			break;
		case(0x9000):
			if(mychip8->V[(opcode&0x0F00)>>8] != mychip8->V[(opcode&0x00F0)>>4]){
				mychip8->pc+=2;
			}
			break;
		case(0xB000):
			mychip8->pc = opcode&0x0FFF + mychip8->V[0];
			break;
		case(0xC000):
			uint8_t random = rand() & 0xFF;			
			mychip8->V[(opcode&0x0F00)>>8] & (opcode & 0x00FF);
			break;
		case(0x3000):
			if((mychip8->V[(opcode&0x0F00)>>8])==(opcode&0x00FF)){
				mychip8->pc+=2;
			}
			break;
		case(0xE000):
			switch(opcode&0x00FF){
				case(0x009E):{
					uint8_t key_check = mychip8->V[(opcode&0x0F00)>>8];	
					if(mychip8->keys[key_check] == 1){
						mychip8->pc+=2;
					}
					break;
				}
				case(0x00A1):{
					uint8_t key_check = mychip8->V[(opcode&0x0F00)>>8];
					if(mychip8->keys[key_check] == 0){
						mychip8->pc+=2;		
					}		
					break;
				}
			}
			break;
		case(0xF000):
			switch(opcode&0x00FF){
				case(0x0007):
					mychip8->V[(opcode&0x0F00)>>8] = mychip8->delay_timer;
					break;
				case(0x000A):
					int key_pressed = 0;
					uint8_t pressed_key;
					for(uint8_t i = 0; i<16 ; i++){
						if(mychip8->keys[i] != 0){
							key_pressed = 1;		
							pressed_key = i;
						}
					}
					if(key_pressed == 1){
						mychip8->V[(opcode&0x0F00)>>8] = pressed_key;
					}
					else{
						mychip8->pc-=2;	
					}
					break;
				case(0x0015):
					mychip8->delay_timer = mychip8->V[(opcode&0x0F00)>>8];
					break;
				case(0x0018):
					mychip8->sound_timer = mychip8->V[(opcode&0x0F00)>>8];
					break;
				case(0x001E):
					mychip8->I+=mychip8->V[(opcode&0x0F00)>>8];
					break;
				case(0x0029):
					uint8_t digit = mychip8->V[(opcode&0x0F00)>>8];
					mychip8->I = digit*5;
					break;	
				case(0x0033):
					int value = mychip8->V[(opcode&0x0F00)>>8];
					mychip8->memory[mychip8->I] = value / 100;
					mychip8->memory[mychip8->I + 1] = (value / 10) % 10;
					mychip8->memory[mychip8->I + 2] = value % 10;
					break;	
				case(0x0055):{
					uint8_t x = (opcode&0x0F00)>>8;
					for(uint8_t i = 0; i<x+1; i++){
						mychip8->V[i] = mychip8->memory[mychip8->I+i];
					}
					break;
				}
				case(0x0065):
					uint8_t x=(opcode&0x0F00)>>8;
					for(int i=0;i<x+1;i++){
						mychip8->memory[mychip8->I+i] = mychip8->V[i];
					}
					break;
			}
			break;
		case(0xD000):
			draw_flag = 1;
			uint8_t n = opcode&0x000F;
			//collision flag
			mychip8->V[0xF]=0;
			uint16_t x_cords = mychip8->V[(opcode&0x0F00)>>8];
			uint16_t y_cords = mychip8->V[(opcode&0x00F0)>>4];
			for(uint8_t i=0; i<n; i++){
				uint8_t byte = mychip8->memory[mychip8->I+i];
				for(uint8_t j=0; j<8; j++){
					uint8_t pixel = byte & (0x80 >> j);
					if(pixel != 0){
						int x_display = (x_cords + j) % 64;
						int y_display = (y_cords + i) % 32;
						int index = (y_display*64) + x_display;
						if(mychip8->display[index] == 1){
							mychip8->V[0xF] = 1;
						}
						mychip8->display[index] ^= 1;
					}
				}
			}
			break;
	}
}

void render_engine(chip8* mychip8){
	//fill with black
	SDL_SetRenderDrawColor(render,225,105,180,255);
	SDL_RenderClear(render);

	for(int y=0; y<32; y++){
		for(int x=0;x<64;x++){
			int index = (y*64)+x;
			if(mychip8->display[index] == 1){ 
				SDL_Rect pixel; 
				pixel.x = x*10;
				pixel.y = y*10;
				pixel.w = 10;
				pixel.h = 10;
				SDL_SetRenderDrawColor(render,255,227,192,255);
				SDL_RenderFillRect(render,&pixel);	
			}
		}	
	SDL_RenderPresent(render);
	}
}

void disassembler(uint16_t opcode, chip8* mychip8){
	printf("[ opcode: %x ] [ pc: %d ] ",opcode, mychip8->pc);

	switch(opcode&0xF000){
		case(0x0000):
			switch(opcode&0x00FF){
				case(0x00E0):
					printf("[ CLS ] ");
					break;
				case(0x00EE):
					printf("[ RET ] ");
					break;	
			}
			break;
		case(0x2000):
			printf("[ CALL addr ]");
			break;	
		case(0x4000):
			printf("[ SNE if Vx != byte ]");
			break;
		case(0x5000):
			printf("[ SE if Vx=Vy ]");
			break;
		case(0xA000):
			printf("[ LD I, addr ]");
			break;
		case(0x6000):
			printf("[ LD Vx, byte ]");
			break;
		case(0x1000):
			printf("[ JP addr ]");
			break;
		case(0x7000):
			printf("[ ADD Vx, byte ]");
			break;
		case(0x8000):
			switch(opcode&0x000F){
				case(0x0001):
					printf("[ OR Vx, Vy ]");
					break;
				case(0x0002):
					printf("[ AND Vx, Vy ]");
					break;
				case(0x0003):
					printf("[ XOR Vx, Vy ]");
					break;	
				case(0x0004):
					printf("[ ADD Vx, Vy ]");
					break;	
				case(0x0005):
					printf("[ SUB Vx, Vy ]");
					break;	
				case(0x0006):
					printf("[ Set Vx = Vx SHR 1 ]");
					break;
				case(0x0007):
					printf("[ SUBN Vx, Vy ]");
					break;
				case(0x000E):
					printf("[ Set Vx=Vx SHL 1 ]");
					break;
			}
			break;
		case(0x9000):
			printf("[ SNE if Vx != Vy ]");
			break;
		case(0xB000):
			printf("[ JP V0, addr ]");
			break;
		case(0xC000):
			printf("[ RND Vx, byte ]");
			break;
		case(0x3000):
			printf("[ SE if Vx = byte ]");
			break;
		case(0xE000):
			switch(opcode&0x00FF){
				case(0x009E):
					printf("[ SKP Vx ]");
					break;
				case(0x00A1):
					printf("[ SKNP Vx ]");	
					break;
			}
			break;
		case(0xF000):
			switch(opcode&0x00FF){
				case(0x0007):
					printf("[ LD Vx, DT ]");
					break;
				case(0x000A):
					printf("[ LD Vx, K ]");
					break;
				case(0x0015):
					printf("[ LD DT, Vx ]");	
					break;
				case(0x0018):
					printf("[ LD ST, Vx ]");
					break;
				case(0x0029):
					printf("[ set I to loc Vx digit ]");
					break;			
				case(0x001E):
					printf("[ ADD I, Vx ]");
					break;
				case(0x0033):
					printf("[ LD B, Vx ]");
					break;
				case(0x0055):
					printf("[ LD I, Vx ]");
					break;
				case(0x0065):
					printf("[ LD Vx, I ]");
					break;
			}
			break;
		case(0xD000):
			printf("[ DRW Vx, Vy, nibble ]");	
			break;
	}
	printf("\n");
}



void sdl_event(chip8* mychip8){
	SDL_Event event;
	while(SDL_PollEvent(&event)){
		switch(event.type){
			case SDL_QUIT:
				mychip8->isrunning = false;	
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
					case SDLK_1: mychip8->keys[0x1] = 1; break;
					case SDLK_2: mychip8->keys[0x2] = 1; break;
					case SDLK_3: mychip8->keys[0x3] = 1; break;
					case SDLK_4: mychip8->keys[0xC] = 1; break;
					case SDLK_q: mychip8->keys[0x4] = 1; break;
					case SDLK_w: mychip8->keys[0x5] = 1; break;
					case SDLK_e: mychip8->keys[0x6] = 1; break;
					case SDLK_r: mychip8->keys[0xD] = 1; break;
					case SDLK_a: mychip8->keys[0x7] = 1; break;
					case SDLK_s: mychip8->keys[0x8] = 1; break;
					case SDLK_d: mychip8->keys[0x9] = 1; break;
					case SDLK_f: mychip8->keys[0xE] = 1; break;
					case SDLK_z: mychip8->keys[0xA] = 1; break;
					case SDLK_x: mychip8->keys[0x0] = 1; break;
					case SDLK_c: mychip8->keys[0xB] = 1; break;
					case SDLK_v: mychip8->keys[0xF] = 1; break;
				}
				break;
			case SDL_KEYUP:
				switch(event.key.keysym.sym){
					case SDLK_1: mychip8->keys[0x1] = 0; break;
					case SDLK_2: mychip8->keys[0x2] = 0; break;
					case SDLK_3: mychip8->keys[0x3] = 0; break;
					case SDLK_4: mychip8->keys[0xC] = 0; break;
					case SDLK_q: mychip8->keys[0x4] = 0; break;
					case SDLK_w: mychip8->keys[0x5] = 0; break;
					case SDLK_e: mychip8->keys[0x6] = 0; break;
					case SDLK_r: mychip8->keys[0xD] = 0; break;
					case SDLK_a: mychip8->keys[0x7] = 0; break;
					case SDLK_s: mychip8->keys[0x8] = 0; break;
					case SDLK_d: mychip8->keys[0x9] = 0; break;
					case SDLK_f: mychip8->keys[0xE] = 0; break;
					case SDLK_z: mychip8->keys[0xA] = 0; break;
					case SDLK_x: mychip8->keys[0x0] = 0; break;
					case SDLK_c: mychip8->keys[0xB] = 0; break;
					case SDLK_v: mychip8->keys[0xF] = 0; break;
				}
				break;
		}
	}

}
