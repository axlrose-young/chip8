#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

typedef struct{
	uint8_t memory[4096];
	uint8_t V[16];
	uint16_t I;
	uint16_t pc;
	bool isrunning;
	int display[64*32];
}chip8;

void load_rom(chip8* mychip8);
void emulate_cycle(chip8* mychip8);
void render(chip8* mychip8);

int main(){
	chip8 mychip8;
	//loading rom
	load_rom(&mychip8);
	mychip8.isrunning = true;
	mychip8.pc = 0x200;
	printf("\033[2J\033[?25l");
	while(mychip8.isrunning){
		emulate_cycle(&mychip8);
		render(&mychip8);
	}
	printf("\033[?25h");
	return 0;
}

void load_rom(chip8* mychip8){
	FILE* pfile = fopen("ibm.ch8","rb");
	if(pfile == NULL){
		perror("Error opening rom\n");
	}
	fread(&mychip8->memory[0x200],1,sizeof(mychip8->memory),pfile);
}

void emulate_cycle(chip8* mychip8){
	uint16_t opcode;
	opcode = (mychip8->memory[mychip8->pc] << 8) | mychip8->memory[mychip8->pc+1];
	if(mychip8->pc >= 4096){
		mychip8->isrunning = false;
	}
	mychip8->pc+=2;

	switch(opcode&0xF000){
		case(0x0000):
			if(opcode == 0x00E0){
				memset(mychip8->display,0,sizeof(mychip8->display));
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
			mychip8->V[(opcode&0x0F00)>>8]+=opcode&0x00FF;
			break;
		case(0x3000):
			if((mychip8->V[opcode&0x0F00]>>8)==opcode&0x00FF){
				mychip8->pc+=2;
			}
			break;
		case(0xF000):
			switch(opcode&0x00FF){
				case(0x001E):
					mychip8->I+=mychip8->V[(opcode&0x0F00)>>8];
					break;
				case(0x0065):
					uint8_t x=(opcode&0x0F00)>>8;
					for(int i=0;i<x+1;i++){
						mychip8->V[i] = mychip8->memory[mychip8->I+i];
					}
					break;
			}
			break;
		case(0xD000):
			uint8_t n = opcode&0x000F;
			//collision flag
			mychip8->V[0xF]=0;
			uint16_t x_cords = mychip8->V[(opcode&0x0F00)>>8];
			uint16_t y_cords = mychip8->V[(opcode&0x00F0)>>4];
			for(int i=0; i<n; i++){
				uint8_t byte = mychip8->memory[mychip8->I+i];
				for(int j=0; j<8; j++){
					uint8_t pixel = byte & (0x80 >> j);
					if(pixel != 0){
						int x_display = x_cords + j % 64;
						int y_display = y_cords + i % 32;
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

void render(chip8* mychip8){
	printf("\033[H");
	for(int y=0; y<32; y++){
		for(int x=0;x<64;x++){
			int index = (y*64)+x;
			if(mychip8->display[index] == 1){
				printf("##");
			}
			else{
				printf("  ");
			}
		}
		printf("\n");
	}
	fflush(stdout);
}
