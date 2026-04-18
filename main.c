/**
 * MPU6050 + SSD1306 - Versão Final
 * Layout otimizado para display 128x64
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdio.h"
#include "ei_bridge.h"

// ================== CONFIGURAÇÕES ==================
// MPU6500 - I2C0
#define I2C0_PORT i2c0
#define MPU_SDA_PIN 4
#define MPU_SCL_PIN 5
#define MPU_ADDR 0x68

// SSD1306 - I2C1
#define I2C1_PORT i2c1
#define OLED_SDA_PIN 14
#define OLED_SCL_PIN 15
#define OLED_ADDR 0x3C

// Dimensões do display
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Buffer do display
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];

// ================== FUNÇÕES SSD1306 ==================

// Enviar comando
void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C1_PORT, OLED_ADDR, buf, 2, false);
}

// Inicialização robusta do SSD1306
void oled_init() {
    sleep_ms(100);
    
    // Sequência de inicialização completa
    oled_cmd(0xAE); // Display OFF
    
    oled_cmd(0xD5); // Set display clock divide ratio/oscillator frequency
    oled_cmd(0x80);
    
    oled_cmd(0xA8); // Set multiplex ratio
    oled_cmd(0x3F); // 64 lines
    
    oled_cmd(0xD3); // Set display offset
    oled_cmd(0x00); // No offset
    
    oled_cmd(0x40); // Set start line address to 0
    
    oled_cmd(0x8D); // Enable charge pump regulator
    oled_cmd(0x14);
    
    oled_cmd(0x20); // Set memory addressing mode
    oled_cmd(0x00); // Horizontal addressing mode
    
    oled_cmd(0xA1); // Set segment re-map (column address 127 mapped to SEG0)
    
    oled_cmd(0xC8); // Set COM output scan direction (scan from COM[N-1] to COM0)
    
    oled_cmd(0xDA); // Set COM pins hardware configuration
    oled_cmd(0x12); // Alternative COM pin configuration
    
    oled_cmd(0x81); // Set contrast control
    oled_cmd(0x7F); // Medium contrast
    
    oled_cmd(0xD9); // Set pre-charge period
    oled_cmd(0xF1);
    
    oled_cmd(0xDB); // Set VCOMH deselect level
    oled_cmd(0x40);
    
    oled_cmd(0xA4); // Entire display ON (resume)
    
    oled_cmd(0xA6); // Set normal display (not inverted)
    
    oled_cmd(0xAF); // Display ON
    
    sleep_ms(100);
    
    // Limpar buffer
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

// Atualizar display
void oled_update() {
    for (uint8_t page = 0; page < 8; page++) {
        oled_cmd(0xB0 + page); // Set page address
        oled_cmd(0x00);        // Set lower column address
        oled_cmd(0x10);        // Set higher column address
        
        uint8_t data[OLED_WIDTH + 1];
        data[0] = 0x40; // Data mode
        
        memcpy(&data[1], &oled_buffer[page * OLED_WIDTH], OLED_WIDTH);
        i2c_write_blocking(I2C1_PORT, OLED_ADDR, data, OLED_WIDTH + 1, false);
    }
}

// Desenhar pixel
void oled_draw_pixel(int x, int y, bool on) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    
    int byte_index = x + (y / 8) * OLED_WIDTH;
    int bit_index = y % 8;
    
    if (on) {
        oled_buffer[byte_index] |= (1 << bit_index);
    } else {
        oled_buffer[byte_index] &= ~(1 << bit_index);
    }
}

// Desenhar caractere 5x7
void oled_draw_char(int x, int y, char c) {
    static const uint8_t font_5x7[] = {
        // Apenas caracteres que usaremos
        0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
        0x00, 0x42, 0x7F, 0x40, 0x00, // 1
        0x42, 0x61, 0x51, 0x49, 0x46, // 2
        0x21, 0x41, 0x49, 0x4D, 0x33, // 3
        0x18, 0x14, 0x12, 0x7F, 0x10, // 4
        0x27, 0x45, 0x45, 0x45, 0x39, // 5
        0x3C, 0x4A, 0x49, 0x49, 0x31, // 6
        0x41, 0x21, 0x11, 0x09, 0x07, // 7
        0x36, 0x49, 0x49, 0x49, 0x36, // 8
        0x46, 0x49, 0x49, 0x29, 0x1E, // 9
        0x7E, 0x09, 0x09, 0x09, 0x7E, // A
        0x7F, 0x49, 0x49, 0x49, 0x36, // B
        0x3E, 0x41, 0x41, 0x41, 0x22, // C
        0x7F, 0x41, 0x41, 0x41, 0x3E, // D
        0x7F, 0x49, 0x49, 0x49, 0x41, // E
        0x7F, 0x09, 0x09, 0x09, 0x01, // F
        0x3E, 0x41, 0x49, 0x49, 0x7A, // G
        0x7F, 0x08, 0x08, 0x08, 0x7F, // H
        0x00, 0x41, 0x7F, 0x41, 0x00, // I
        0x20, 0x40, 0x41, 0x3F, 0x01, // J
        0x7F, 0x08, 0x14, 0x22, 0x41, // K
        0x7F, 0x40, 0x40, 0x40, 0x40, // L
        0x7F, 0x02, 0x0C, 0x02, 0x7F, // M
        0x7F, 0x04, 0x08, 0x10, 0x7F, // N
        0x3E, 0x41, 0x41, 0x41, 0x3E, // O
        0x7F, 0x09, 0x09, 0x09, 0x06, // P
        0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
        0x7F, 0x09, 0x09, 0x19, 0x66, // R
        0x46, 0x49, 0x49, 0x49, 0x31, // S
        0x01, 0x01, 0x7F, 0x01, 0x01, // T
        0x3F, 0x40, 0x40, 0x40, 0x3F, // U
        0x1F, 0x20, 0x40, 0x20, 0x1F, // V
        0x3F, 0x40, 0x38, 0x40, 0x3F, // W
        0x63, 0x14, 0x08, 0x14, 0x63, // X
        0x07, 0x08, 0x70, 0x08, 0x07, // Y
        0x61, 0x51, 0x49, 0x45, 0x43, // Z
        0x00, 0x36, 0x36, 0x00, 0x00, // :
        0x00, 0x00, 0x00, 0x00, 0x00, // Espaço
        0x2A, 0x1C, 0x7F, 0x1C, 0x2A, // *
    };
    
    // Mapeamento de caracteres para índice
    int index = -1;
    
    if (c >= '0' && c <= '9') {
        index = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        index = 10 + (c - 'A');
    } else if (c >= 'a' && c <= 'z') {
        index = 10 + (c - 'a'); // Minúsculas usam mesma fonte
    } else if (c == ':') {
        index = 36;
    } else if (c == ' ') {
        index = 37;
    } else if (c == '*') {
        index = 38;
    }
    
    if (index >= 0 && index <= 38) {
        const uint8_t *char_data = &font_5x7[index * 5];
        
        for (int col = 0; col < 5; col++) {
            uint8_t col_data = char_data[col];
            for (int row = 0; row < 7; row++) {
                bool pixel = (col_data >> row) & 1;
                oled_draw_pixel(x + col, y + row, pixel);
            }
        }
    }
}

// Desenhar string
void oled_draw_string(int x, int y, const char *str) {
    int start_x = x;
    while (*str) {
        oled_draw_char(start_x, y, *str);
        start_x += 6; // 5 pixels + 1 de espaçamento
        str++;
        
        // Quebra de linha se necessário
        if (start_x > OLED_WIDTH - 6) {
            start_x = x;
            y += 8;
        }
    }
}

// Desenhar linha horizontal
void oled_draw_hline(int x, int y, int w) {
    for (int i = 0; i < w; i++) {
        oled_draw_pixel(x + i, y, true);
    }
}

// Desenhar linha vertical
void oled_draw_vline(int x, int y, int h) {
    for (int i = 0; i < h; i++) {
        oled_draw_pixel(x, y + i, true);
    }
}

// Limpar display
void oled_clear() {
    memset(oled_buffer, 0, sizeof(oled_buffer));
}

// ================== FUNÇÕES MPU6500 ==================

// Inicializar MPU6500
static void mpu_init() {
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(I2C0_PORT, MPU_ADDR, buf, 2, false);
    sleep_ms(100);
    
    buf[1] = 0x00;  // Clear sleep mode
    i2c_write_blocking(I2C0_PORT, MPU_ADDR, buf, 2, false);
    sleep_ms(10);
}

// Ler dados do acelerômetro e temperatura
static void mpu6500_read_raw(int16_t accel[3], int16_t *temp) {
    uint8_t buffer[6];
    
    // Ler aceleração (6 bytes a partir do registrador 0x3B)
    uint8_t val = 0x3B;
    i2c_write_blocking(I2C0_PORT, MPU_ADDR, &val, 1, true);
    i2c_read_blocking(I2C0_PORT, MPU_ADDR, buffer, 6, false);

    for (int i = 0; i < 3; i++) {
        accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
    }

    // Ler temperatura (2 bytes a partir do registrador 0x41)
    val = 0x41;
    i2c_write_blocking(I2C0_PORT, MPU_ADDR, &val, 1, true);
    i2c_read_blocking(I2C0_PORT, MPU_ADDR, buffer, 2, false);

    *temp = buffer[0] << 8 | buffer[1];
}

// Desenhar barra gráfica com zero no centro
void draw_bar(int x, int y, int width, int height, float value, float min_val, float max_val) {
    // Desenhar borda
    oled_draw_hline(x, y, width);
    oled_draw_hline(x, y + height - 1, width);
    oled_draw_vline(x, y, height);
    oled_draw_vline(x + width - 1, y, height);
    
    // Linha central (zero)
    int center_y = y + height / 2;
    oled_draw_hline(x + 1, center_y, width - 2);
    
    // Calcular preenchimento baseado no valor
    float range = max_val - min_val;
    float normalized = (value - min_val) / range;
    
    // Garantir que o valor está dentro dos limites
    if (normalized < 0) normalized = 0;
    if (normalized > 1) normalized = 1;
    
    int bar_height = normalized * height;
    int fill_height;
    
    if (value >= 0) {
        // Aceleração positiva: preencher de cima para baixo a partir do centro
        fill_height = bar_height / 2;
        if (fill_height > 0) {
            for (int i = 0; i < fill_height; i++) {
                oled_draw_hline(x + 1, center_y - i, width - 2);
            }
        }
    } else {
        // Aceleração negativa: preencher de baixo para cima a partir do centro
        fill_height = (height - bar_height) / 2;
        if (fill_height > 0) {
            for (int i = 0; i < fill_height; i++) {
                oled_draw_hline(x + 1, center_y + i, width - 2);
            }
        }
    }
}

// ================== MAIN ==================

int main() {
    // Inicializar serial
    stdio_init_all();
    
    // Aguardar serial estabilizar
    sleep_ms(500);
    
    printf("\n=== MPU6500 + SSD1306 ===\n");
    printf("Inicializando...\n");
    printf("EI ping: %d\n", ei_ping());
    
    // Inicializar I2C0 para MPU6500
    i2c_init(I2C0_PORT, 100000); // 100 kHz
    gpio_set_function(MPU_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(MPU_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(MPU_SDA_PIN);
    gpio_pull_up(MPU_SCL_PIN);
    
    // Inicializar I2C1 para SSD1306
    i2c_init(I2C1_PORT, 400000); // 400 kHz
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);
    
    printf("I2C inicializado\n");

    // Inicializar Acelerômetro
    mpu_init();

    // Inicializar display
    oled_init();
    printf("Display OK\n");
    
    // Tela inicial
    oled_clear();
    oled_draw_string(30, 10, "PICO W");
    oled_draw_string(10, 25, "MPU6050+SSD1306");
    oled_draw_hline(0, 40, 128);
    oled_draw_string(20, 45, "SENSOR OK");
    
    oled_update();
    sleep_ms(2000);
    
    // Variáveis principais
    int16_t raw_accel[3], raw_temp;
    float accel_g[3], temp_c;
    char buffer[20];
    uint32_t counter = 0;
    uint32_t last_update = 0;
    const char *last_label = "OK";
    int last_score_pct = 0;
    
    while (1) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        
        // Atualizar a cada 100ms (10Hz)
        if (now - last_update >= 100) {
            counter++;
            last_update = now;
            
            // Ler dados do sensor
            mpu6500_read_raw(raw_accel, &raw_temp);
            
            // Converter aceleração para g
            accel_g[0] = raw_accel[0] / 16384.0;
            accel_g[1] = raw_accel[1] / 16384.0;
            accel_g[2] = raw_accel[2] / 16384.0;

            ei_result_brief_t pred;

            if (ei_add_sample(accel_g[0], accel_g[1], accel_g[2]) == 1) {
                if (ei_predict(&pred) == 0) {
                    last_label = pred.label;
                    last_score_pct = (int)(pred.score * 100.0f + 0.5f);
                    printf("PRED: %s (%d)\n", last_label, last_score_pct);
                }
            }
            
            // Converter temperatura para Celsius
            temp_c = (raw_temp / 340.0) + 36.53;
            
            // ===== ATUALIZAR DISPLAY =====
            oled_clear();
            
            // Cabeçalho
            oled_draw_string(35, 0, "TinyML FIAP");
            oled_draw_hline(0, 9, 128);
            
            // Valores X, Y, Z
            oled_draw_string(5, 12, "X");
            oled_draw_string(5, 22, "Y");
            oled_draw_string(5, 32, "Z");
            
            // Valores numéricos da aceleração
            snprintf(buffer, sizeof(buffer), "%+6.3f", accel_g[0]);
            oled_draw_string(15, 12, buffer);
            
            snprintf(buffer, sizeof(buffer), "%+6.3f", accel_g[1]);
            oled_draw_string(15, 22, buffer);
            
            snprintf(buffer, sizeof(buffer), "%+6.3f", accel_g[2]);
            oled_draw_string(15, 32, buffer);
            
            // Unidade
            oled_draw_string(70, 12, "g");
            oled_draw_string(70, 22, "g");
            oled_draw_string(70, 32, "g");
            
            // Barras gráficas
            draw_bar(80, 12, 12, 20, accel_g[0], -2.0, 2.0);
            draw_bar(95, 12, 12, 20, accel_g[1], -2.0, 2.0);
            draw_bar(110, 12, 12, 20, accel_g[2], -2.0, 2.0);
            
            // Legendas das barras
            oled_draw_string(83, 34, "X");
            oled_draw_string(98, 34, "Y");
            oled_draw_string(113, 34, "Z");
            
            // Linha separadora
            oled_draw_hline(0, 42, 128);
            
            // Rodapé com informações
            oled_draw_string(2, 46, "CNT:");
            snprintf(buffer, sizeof(buffer), "%lu", counter);
            oled_draw_string(28, 46, buffer);
            
            oled_draw_string(60, 46, "TEMP:");
            snprintf(buffer, sizeof(buffer), "%2.0fC", temp_c);
            oled_draw_string(95, 46, buffer);
            
            // Atualizar display
            oled_draw_string(2, 56, "ML:");
            snprintf(buffer, sizeof(buffer), "%s %d", last_label, last_score_pct);
            oled_draw_string(28, 56, buffer);
            oled_update();
            
            // ===== SAÍDA SERIAL =====
            //printf("#%lu: X=%+7.3fg Y=%+7.3fg Z=%+7.3fg \n",
            //       counter, accel_g[0], accel_g[1], accel_g[2]);
            printf("%+7.3f %+7.3f %+7.3f \n",
                     accel_g[0], accel_g[1], accel_g[2]);
        }
        
        // Pequena pausa
        sleep_ms(10);
    }
    
    return 0;
}