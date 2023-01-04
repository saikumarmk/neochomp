

/*--------------------- MATRIX GPIO CONFIG  -------------------------*/
#define R1_PIN 22
#define B1_PIN 19
#define R2_PIN 23
#define B2_PIN 18
#define A_PIN 12
#define C_PIN 4
#define CLK_PIN 16
#define OE_PIN 17



#define G1_PIN 21

#define G2_PIN 2

#define B_PIN 0 
#define D_PIN 26
#define LAT_PIN 25


#define E_PIN -1  //21 // required for 1/32 scan panels, like 64x64. Any available pin would do, i.e. IO32



MatrixPanel_I2S_DMA *dma_display = nullptr;

// Module configuration
HUB75_I2S_CFG::i2s_pins _pins={R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
HUB75_I2S_CFG mxconfig(
  64,   // module width
  32,   // module height
  1,    // Chain length
  _pins
);
