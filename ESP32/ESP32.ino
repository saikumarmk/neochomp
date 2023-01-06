//This code is intended for use in the Arduino IDE, install dependencies as required
#include <ESP32DMASPISlave.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>


// SPI buffer allocation
// SPI buffer slightly larger than frame size to accomate for signal misalignment
// Buffer size must be a multiple of 4 bytes
ESP32DMASPI::Slave slave;
static const uint32_t BUFFER_SIZE = 4200;
uint8_t* spi_slave_tx_buf = NULL;
uint8_t* spi_slave_rx_buf;


// Single frame stored in memory
// Each pixel is a 16 bit number
// RRRRR - GGGGGG - BBBBB
#define FRAME_SIZE 2048
uint16_t frame[FRAME_SIZE];

// Dual core task handles
static TaskHandle_t Task2 = 0;
static TaskHandle_t task_handle_wait_spi = 0;
static TaskHandle_t task_handle_process_buffer = 0;

SemaphoreHandle_t xBinarySemaphore;


// LED panel pin configurations
#include "config.h"


void setup() {
  xBinarySemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(xBinarySemaphore);

  // Set up ready pin, HIGH blocks SPI on the host
  pinMode(1, OUTPUT);
  digitalWrite(1,LOW);

  //SPI setting and initialisation
  spi_slave_rx_buf = slave.allocDMABuffer(BUFFER_SIZE);
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);
  slave.setDataMode(SPI_MODE3); // for DMA, only 1 or 3 is available
  slave.setMaxTransferSize(BUFFER_SIZE);
  slave.setDMAChannel(1); // 1 or 2 only
  slave.setQueueSize(1); // transaction queue size
  // begin() after setting
  // HSPI = CS: 15, CLK: 14, MOSI: 13, MISO: 12
  slave.begin(HSPI); //, 15, 14, 13, -1

  //Multi core task declarations
  //                       task,                name,   stack size, NULL, priority, task handle,                  core number
  xTaskCreatePinnedToCore(task_wait_spi,        "W",    2048,       NULL, 2,        &task_handle_wait_spi,        0           );
  xTaskCreatePinnedToCore(task_process_buffer,  "P",    20000,      NULL, 2,        &task_handle_process_buffer,  0           );
  xTaskCreatePinnedToCore(render,               "R",    20000,      NULL, 6,        &Task2,                       1           );

  xTaskNotifyGive(task_handle_wait_spi);

  digitalWrite(1,LOW);
}

bool hasData = false;

// -task_wait_spi- waits for a SPI packet and notifies -task_process_buffer-
void task_wait_spi(void *pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        slave.wait(spi_slave_rx_buf, BUFFER_SIZE); 
        xTaskNotifyGive(task_handle_process_buffer);
    }
}


// -task_process_buffer- stores the SPI buffer into -frame[]- in memory,
// and then notifies -render-
void task_process_buffer(void *pvParameters)
{
    while(1)
    {
        
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      

      xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
      
        
        for (int i = 0; i < 2048; i++)
        {
          frame[i] = spi_slave_rx_buf[2*i+1] << 8 | spi_slave_rx_buf[2*i];
        }

        slave.pop();
          
        xSemaphoreGive(xBinarySemaphore);

        hasData = true;
        xTaskNotifyGive(Task2);

        xTaskNotifyGive(task_handle_wait_spi);

    }
}






// -render- pushes -frame[]- to the LED panel
// asserts pin 1 while rendering to stop SPI signals from host
void render(void * pvParameters)
{
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(100); //0-255
  while(1)
  {
      xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);

      if(hasData)
      {
        GPIO.out_w1ts = (0b000000000000000000000000000010); //(uint32_t)
        //digitalWrite(1, HIGH);
        for(int i = 0; i < 2048; i++)
        {
          dma_display->drawPixel((  i%64   ), i/64, frame[i]);
        }
        //digitalWrite(1, LOW);
        GPIO.out_w1tc = (0b000000000000000000000000000010);
        hasData = false;
      }
      xSemaphoreGive(xBinarySemaphore);

      delay(1);

  }
}


void loop() {}
