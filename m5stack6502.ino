/*
  Small 6502 emulator for the m5stack
  (c) 2020 olav@schettler.net
  originally 
  - for the blue pill
  - (c) 2019 rampa@encomix.org
  - using:
    jeeH (from jeelabs)
    fake6502 (Mike Chambers)
    Some (ACIA code) from satoshinm
*/
#include <M5Stack.h>

extern "C" {
#include "fake6502.h"
}

void term_init();
void term_write(byte data);
int term_available();
char term_read();

#include "rom.h"
#define ACIAControl 0
#define ACIAStatus 0
#define ACIAData 1

// "MC6850 Data Register (R/W) Data can be read when Status.Bit0=1, and written when Status.Bit1=1."
#define RDRF (1 << 0)
#define TDRE (1 << 1)


static char input[256];
static int input_index = 0;
static int input_processed_index = 0;

void 
process_serial_input_byte(char b) {
  input[input_index++] = b;
  input_index %= sizeof(input);
}


uint8_t 
read6850(uint16_t address) {
	switch(address & 1) {
		case ACIAStatus: {
      // Always writable
      uint8_t flags = TDRE;

      // Readable if there is pending user input data which wasn't read
      if (input_processed_index < input_index) flags |= RDRF;

      return flags;
			break;
    }
		case ACIAData: {
      char data = input[input_processed_index++];
      input_processed_index %= sizeof(input);
      return data;
			break;
    }
		default:
      break;
	}

	return 0xff;
}

void 
write6850(uint16_t address, uint8_t value) {
  //printf("ACIA address: %x  value %x",address,value);
	switch(address & 1) {
		case ACIAControl:
      // TODO: decode baudrate, mode, break control, interrupt
			break;
		case ACIAData: {
      Serial.write(value);
      term_write(value);
			break;
    }
		default:
      break;
	}
}

uint8_t ram[0x4000];
uint8_t 
read6502(uint16_t address) {
  // RAM
  if (address < sizeof(ram)) {
    return ram[address];
  }

  // ROM
  if (address >= 0xc000) {
    return rom[address - 0xc000];
  }

  // ACIA
  if (address >= 0xa000 && address <= 0xbfff) {
    return read6850(address);
  }

  return 0xff;
}

void 
write6502(uint16_t address, uint8_t value) {
  // RAM
  if (address < sizeof(ram)) {
    ram[address] = value;
  }

  // ACIA
  if (address >= 0xa000 && address <= 0xbfff) {
    write6850(address, value);
  }
}

void setup() {
  term_init();
  
  Serial.begin(115200);

  Serial.println("6502 reset...");
  reset6502();
  Serial.println("6502 Starting...");
}

void loop() {
  while (true) {
    if (Serial.available()) {
      process_serial_input_byte(Serial.read());
    }
    if (term_available()) {
      char c = term_read();
      if (c != 0) {
        process_serial_input_byte(c);
      }
    }
    
    step6502();
  }
}
