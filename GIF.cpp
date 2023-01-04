#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "easywsclient.hpp"
#include "easywsclient.cpp"
#include <assert.h>

#include "mraa/spi.h"
#include "mraa/gpio.h"

#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define SPI_BUS 0 

#define SPI_FREQ 12000000


mraa_spi_context spi;
mraa_gpio_context CS;
mraa_gpio_context r1;
mraa_gpio_context r2;
mraa_gpio_context RDY;
mraa_gpio_context BTN;

clock_t start, end;
double time_used;


using easywsclient::WebSocket;
static WebSocket::pointer ws = NULL;



void sendFrame(uint8_t frame[])
{
	mraa_gpio_write(CS,0);

	free(mraa_spi_write_buf(spi, frame, 4096));

	mraa_gpio_write(CS,1);
}



int to5(int value)
{
	return value*31/255;
}

int to6(int value)
{
	return value*63/255;
}




void preRender(unsigned int frame_n, uint8_t output[], unsigned char *input)
{
	for(int i = 0; i < frame_n; i++)
	{
		for(int j = 0; j < 2048; j++)
		{
			unsigned char* pixelOffset = input + (int) (j%64 + 64 * floor(j/64) + i * 2048) * 3;

			output[ j 	  + 6144 * i ] = 	(uint8_t) to5( pixelOffset[0] );
			output[ j + 2048  + 6144 * i ] = 	(uint8_t) to6( pixelOffset[1] );
			output[ j + 4096  + 6144 * i ] = 	(uint8_t) to5( pixelOffset[2] );
		}
	}
}


unsigned char* dataGIF;
uint8_t* rawFrames; 
int GIFLength;

void loadGIF(const char* GIFName)
{
	FILE* pf = fopen(GIFName, "rb");
	fseek(pf, 0L, SEEK_END);
	int size = (int) ftell(pf);
	void*buffer = malloc(size);
	fseek(pf, 0L, SEEK_SET);
	fread(buffer, size, 1, pf);
	fclose(pf);

	int *delays = nullptr;
	int x,y,z,comp2;
	
	dataGIF = stbi_load_gif_from_memory( (stbi_uc*) buffer, size, &delays, &x,&y,&z, &comp2, 3);

	printf("%d\n", z);

	free(buffer);

	delete[] rawFrames;
	rawFrames = new uint8_t[z*2048*3];

	preRender(z, rawFrames, dataGIF);
	GIFLength = z;
}



void handle_message(const std::string & message)
{
	printf("        %s\n", message.c_str());
	loadGIF(message.c_str());
}




int main(int argc, char** argv)
{
	sleep(3);

	ws = WebSocket::from_url("ws://localhost:8080/alternate");
	assert(ws);
	ws->send("hello");


	mraa_deinit();
	mraa_init();

	spi = mraa_spi_init(SPI_BUS);
	
	if(spi == NULL)
	{
		printf("FAIL SPI\n");
	}

	mraa_spi_frequency(spi, SPI_FREQ);
	mraa_spi_mode(spi, MRAA_SPI_MODE3);
	mraa_spi_bit_per_word(spi, 8);

	CS = mraa_gpio_init(19);
	mraa_gpio_dir(CS, MRAA_GPIO_OUT);

	mraa_gpio_write(CS, 1);

	
	r1 = mraa_gpio_init(7);
	r2 = mraa_gpio_init(11);
	RDY = mraa_gpio_init(22);
	BTN = mraa_gpio_init(21);
	
	mraa_gpio_dir(r1, MRAA_GPIO_IN);
	mraa_gpio_dir(r2, MRAA_GPIO_IN);
	mraa_gpio_dir(RDY, MRAA_GPIO_IN);
	mraa_gpio_dir(BTN, MRAA_GPIO_IN);


	loadGIF("sulemio.gif");
	
	fflush(stdout);
	uint8_t val[4096];
	
	clock_t rotaryTime = clock();
	bool lastA = mraa_gpio_read(r1);
	bool runLoop = true;
	bool debounceFlag = true;

	float rotCount = 0.8;

	int j = 0;
		
	clock_t frameTime = clock();
	clock_t btnTime = clock();
	clock_t transactionTime = 0;
	clock_t wsTime = clock();

	while(1){
		ws->poll();
		ws->dispatch(handle_message);


		if( true )
		{	

			rotaryTime = clock();
			bool a = mraa_gpio_read(r1);
			bool b = mraa_gpio_read(r2);
			
			if( a != lastA && a == 1 )
			{
				if( b!= a)
				{
					//rotCount = rotCount < 0.91 ? rotCount + 0.01 : 1;
				}
				else
				{
					//rotCount = rotCount - 0.01 > 0 ? rotCount - 0.01 : 0;
				}

				printf("%.2f\n", rotCount);
			}

			lastA = a;

		}
		

		if(	mraa_gpio_read(BTN) == 0 && debounceFlag )
		{
			ws->send("BUTTON");
			runLoop = !runLoop;
			btnTime = clock();
			debounceFlag = false;
		}

		if(debounceFlag == false && clock() - btnTime > 5000 && mraa_gpio_read(BTN) )
		{
			debounceFlag = true;
		}



		
		if(clock() - frameTime + transactionTime > 40000)
		{
			frameTime = clock();

		j ++;

		if(j >= GIFLength)
		{
			j = 0;
		}
		

			start = clock();
			for(int i = 0; i < 2048; i++)
			{
				unsigned char r = rawFrames[i  +  6144 * j]	  * 1	;
				unsigned char g = rawFrames[i  +  6144 * j + 2048]* 1	;
				unsigned char b = rawFrames[i  +  6144 * j + 4096]* 1	;

				r = r - r*0.05	;
				g = g - g*0.05	;
				b = b - b*0.05	;

				val[2*i+1] = r << 3 | g >> 3;
				val[2*i] = (g & 0b000111) << 5 | b;
	
			}
			end = clock();
				
			if( mraa_gpio_read(RDY) == 0 )
			{
				ws->send(std::to_string(j));
				//printf("%d\n", j);
				sendFrame(val);
			}
			transactionTime = clock() - frameTime;
		}


	}



	mraa_spi_stop;



	return 0;
}





