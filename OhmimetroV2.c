#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "pico/bootrom.h"
#include "ws2812.pio.h"

#define I2C_PORT    i2c1
#define I2C_SDA     14
#define I2C_SCL     15
#define OLED_ADDR   0x3C
#define ADC_PIN     28     // GPIO para o voltímetro
#define BTN_B       6      // Botão B para BOOTSEL

// Configuração da matriz de LEDs WS2812
#define IS_RGBW     false
#define NUM_PIXELS  25
#define WS2812_PIN  7


static const int R_CONHECIDO = 10000;       // Resistência conhecida (10 kΩ)
static const int ADC_RESOLUTION = 4095;     // Resolução do ADC (12 bits)

// Série E24: 24 valores base
static const int E24_BASE[24] = {10,11,12,13,15,16,18,20,22,24,27,30,33,36,39,43,47,51,56,62,68,75,82,91};
#define E24_BASE_COUNT 24
#define MIN_DECade     1        // 10^1 = 10
#define MAX_DECade     4        // 10^4 = 10000
#define MIN_VAL_OHM    510      // faixa mínima
#define MAX_VAL_OHM    100000

static int e24_list[64];
static int e24_count = 0;

// Nomes das faixas de cores
static const char *color_names[10] = {"Preto","Marrom","Vermelho","Laranja","Amarelo","Verde","Azul","Violeta","Cinza","Branco"};

// Definição das cores RGB para cada cor do código de cores
// Algumas cores não apreseentam uma boa representação em RGB
static const uint8_t colors[10][3] = {
    {0, 0, 0},       // Preto
    {50, 10, 0},     // Marrom
    {50, 0, 0},      // Vermelho
    {50, 25, 0},     // Laranja
    {50, 50, 0},     // Amarelo
    {0, 50, 0},      // Verde
    {0, 0, 50},      // Azul
    {30, 0, 50},     // Violeta
    {30, 30, 30},    // Cinza
    {50, 50, 50}     // Branco
};

// Buffer para a matriz de LEDs
bool led_matrix[NUM_PIXELS];

// Inicializa lista de valores E24
static void init_e24_list() {
    e24_count = 0;
    for (int i = 0; i < E24_BASE_COUNT; i++) {
        for (int d = MIN_DECade; d <= MAX_DECade; d++) {
            int val = E24_BASE[i];
            for (int p = 0; p < d; p++) val *= 10;
            if (val >= MIN_VAL_OHM && val <= MAX_VAL_OHM) {
                e24_list[e24_count++] = val;
            }
        }
    }
}

// Encontra o valor nominal E24 mais próximo, arredondando a medição para o valor padrão mais próximo
static int find_nearest_nominal(float Rx) {
    float best_diff = 1e9f;
    int best_val = MIN_VAL_OHM;
    for (int i = 0; i < e24_count; i++) {
        float diff = fabsf(Rx - (float)e24_list[i]);
        if (diff < best_diff) {
            best_diff = diff;
            best_val = e24_list[i];
        }
    }
    return best_val;
}

// Interrupção para BOOTSEL
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

// Funções para controle dos LEDs WS2812
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Função para exibir um padrão de cores na matriz de LEDs
void display_color_pattern(bool *buffer, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = urgb_u32(r, g, b);
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(buffer[i] ? color : 0);
    }
}

// Função para limpar todos os LEDs
void clear_leds() {
    for (int i = 0; i < NUM_PIXELS; i++) {
        put_pixel(0);
    }
}

// Exibe na matriz de LEDs as cores das faixas de cores eem formato de coluna
void display_colors_matriz(int d1, int d2, int expo) {
    
    for (int i = 0; i < NUM_PIXELS; i++) {
       
        uint32_t color = 0;  // Por padrão, LED apagado
        
        // Calcula a linha e coluna lógicas (como se fosse uma matriz normal)
        int row = i / 5;
        int col;
        
        // Ajusta a coluna com base no padrão de serpentina
        if (row % 2 == 0) {
            // Linhas pares (0, 2, 4) - da esquerda para a direita
            col = i % 5;
        } else {
            // Linhas ímpares (1, 3) - da direita para a esquerda
            col = 4 - (i % 5);
        }
        
        // Agora atribui as cores com base na coluna lógica
        if (col == 3) {
            // Coluna 3 - Primeira faixa (d1)
            color = urgb_u32(colors[d1][0], colors[d1][1], colors[d1][2]);
        }
        else if (col == 2) {
            // Coluna 2 - Segunda faixa (d2)
            color = urgb_u32(colors[d2][0], colors[d2][1], colors[d2][2]);
        }
        else if (col == 1) {
            // Coluna 1 - Multiplicador (expo)
            color = urgb_u32(colors[expo][0], colors[expo][1], colors[expo][2]);
        }
        
        // Envia a cor para o LED
        put_pixel(color);
    }
}

int main() {
    // Configura botão B para BOOTSEL
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // I2C para OLED
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t oled;
    ssd1306_init(&oled, WIDTH, HEIGHT, false, OLED_ADDR, I2C_PORT);
    ssd1306_config(&oled);
    ssd1306_fill(&oled, false);
    ssd1306_send_data(&oled);

    // ADC
    adc_init();
    adc_gpio_init(ADC_PIN);

    // Inicializa série E24
    init_e24_list();

    // Inicializa WS2812 (matriz de LEDs)
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    
    // Limpa todos os LEDs no início
    clear_leds();

    char buf_adc[16];   // ADC: xxxx
    char buf_res[16];   // Resist: xxxx
    char buf_c1[16];    // 1ª Faixa
    char buf_c2[16];    // 2ª Faixa
    char buf_c3[16];    // Multiplicador

    while (true) {
        // Leitura do ADC (média de 500 amostras)
        adc_select_input(2);
        float soma = 0;
        for (int i = 0; i < 500; i++) {
            soma += adc_read();
            sleep_ms(1);
        }
        float media = soma / 500.0f;
        // Cálculo de R_x
        float R_x = (R_CONHECIDO * media) / (ADC_RESOLUTION - media);

        sprintf(buf_adc, "ADC: %1.0f", media);      // Formata leitura do ADC
        sprintf(buf_res, "Resist: %1.0f", R_x);     // Formata resistência medida

        // Nominal E24 e extração de faixas
        int nominal = find_nearest_nominal(R_x);
        int expo = 0;
        int div = nominal;
        while (div >= 100) { div /= 10; expo++; }
        int d1 = div / 10;
        int d2 = div % 10;
        const char *c1 = color_names[d1];
        const char *c2 = color_names[d2];
        const char *c3 = color_names[expo];

        // Formata strings de cor
        sprintf(buf_c1, "1: %s", c1);
        sprintf(buf_c2, "2: %s", c2);
        sprintf(buf_c3, "M: %s", c3);

        ssd1306_fill(&oled, !true);                                 // Limpa display
        ssd1306_rect(&oled, 3, 3, WIDTH-6, HEIGHT-6, true, false);  // Borda externa
        ssd1306_line(&oled, 3, 16, WIDTH-4, 16, true);              // Linhas de separação
        ssd1306_line(&oled, 3, 32, WIDTH-4, 32, true);              // Linhas de separação

       // Exibe ADC, Resistência e Cores
       ssd1306_draw_string(&oled, buf_adc, 8, 4);
       ssd1306_draw_string(&oled, buf_res, 8, 20);
       ssd1306_draw_string(&oled, buf_c1,  8, 36);
       ssd1306_draw_string(&oled, buf_c2,  8, 44);
       ssd1306_draw_string(&oled, buf_c3,  8, 52);

       ssd1306_send_data(&oled);

       // Exibe as três faixas de cores simultaneamente
       display_colors_matriz(d1, d2, expo);

       // Pequena pausa antes de reiniciar
       sleep_ms(500);
    }
    return 0;
}
