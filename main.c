#include <stdio.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <string.h>

int main(){
	uint8_t memory[4096];
	uint16_t opcode;
	uint16_t pc;
	uint8_t display[64 * 32] = {0};
	uint8_t V[16];
	uint16_t I;

	SDL_Window* window =  SDL_CreateWindow("Chip-8",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,640,320,0);
	
	SDL_Renderer* render =  SDL_CreateRenderer(window,-1, 0);
	SDL_SetRenderDrawColor(render,0, 0, 0,SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render);
	SDL_Delay(3000);

	FILE* pfile = fopen("ibm.ch8","rb");
	if(pfile == NULL){
		printf("Error loading ROM\n");
		return 1;
	}
	fread(&memory[0x200], 1, 4096 - 0x200, pfile);
	pc = 0x200;
	while(pc < 4096){
		opcode = (memory[pc] << 8) | memory[pc+1];
		printf("PC: 0x%03X | Opcode: 0x%04X\n", pc, opcode);
		pc+=2;

		switch(opcode & 0xF000){
			case(0x0000):
				if(opcode == 0x00E0){
					memset(display, 0, sizeof(display));
					
					SDL_SetRenderDrawColor(render, 0, 0, 0,SDL_ALPHA_OPAQUE);
					SDL_RenderClear(render);

					break;
				}
			case(0x1000):
				pc = opcode & 0x0FFF;
				break;
			case(0x6000):
				V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
				break;
			case(0x7000):
				V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
				break;
			case(0xA000):
				I = opcode & 0x0FFF;
				break;
			case(0xD000):
				uint8_t x_start = V[(opcode & 0x0F00) >> 8];
				uint8_t y_start = V[(opcode & 0x00F0) >> 4];
				uint8_t height = opcode & 0x000F;
				V[0xF] = 0;

				for(int row=0; row < height; row++){
					uint8_t sprite_row = memory[I + row];
					for(int col = 0; col < 8; col++){
						uint8_t pixel = sprite_row & (0x80 >> col);
						if(pixel !=0){
							int x = (x_start + col)%64;
							int y = (y_start + row)%32;
							int display_index = x + (y*64);
							if(display[display_index] == 1){
								V[0xF] = 1;
							}
							display[display_index] ^= 1;

							SDL_Rect block;
							block.x = x*10;
							block.y = y*10;
							block.w = 10;
							block.h = 10;
							if(display[display_index] == 1){
								SDL_SetRenderDrawColor(render,255,255,255,255);
							}
							else{
								SDL_SetRenderDrawColor(render,0,0,0,255);
							}
							SDL_RenderFillRect(render, &block);
							SDL_Delay(10);
						}
					}
				}
				SDL_RenderPresent(render);
				break;
			case(0x3000):
				if(V[(opcode & 0x0F00) >> 8] == opcode & 0x00FF){
					pc+=2;
				}
				break;
			case(0xF000):
				if((opcode & 0x00FF) == 0x001E){
					I += V[(opcode & 0x0F00) >> 8];
				}
				if((opcode & 0x00FF) == 0x0065){
					int x = (opcode & 0x0F00) >> 8;
					for(int i=0; i < x+1 ; i++){
						V[i] = memory[I + i];
					}
				}
				break;
		}
	}
	fclose(pfile);
	return 0;
}
