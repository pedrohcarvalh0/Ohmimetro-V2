# Ohmímetro com Display OLED e Matriz de LEDs WS2812

Este projeto utiliza um microcontrolador **RP2040** na placa **BitDogLab** para construir um **Ohmímetro** (medidor de resistência) com exibição de resultados em um **display OLED SSD1306** e visualização de faixas de cores em uma **matriz de LEDs WS2812**.

## Funcionalidades

- **Leitura de resistência** utilizando um resistor conhecido e um ADC para medir a resistência desconhecida.
- **Exibição do valor da resistência** em formato numérico no **display OLED**.
- **Exibição das cores correspondentes** ao valor da resistência medido, de acordo com o código de cores de resistores (padrão E24).
- **Matriz de LEDs WS2812** exibe o padrão de cores, onde cada coluna representa uma faixa de cor do código de cores do resistor (d1, d2, multiplicador).
- **Botão para BOOTSEL**: Um botão dedicado para reiniciar o sistema utilizando a funcionalidade BOOTSEL do RP2040.

## Funcionalidades do Código

1. **Leitura de ADC**:
   - A medição da resistência é realizada utilizando o ADC (entrada GPIO 28), onde um resistor de valor conhecido é comparado com a resistência desconhecida.

2. **Exibição no Display OLED**:
   - A tela exibe o valor lido do ADC, a resistência calculada e as cores associadas ao valor da resistência (em formato de código de cores E24).
   - As cores são exibidas no display utilizando as bibliotecas `ssd1306` e `font`.

3. **Matriz de LEDs WS2812**:
   - O código utiliza uma matriz de LEDs WS2812 (25 LEDs) para exibir um padrão de cores baseado nas faixas do código de cores de resistores.

4. **Reinicialização via Botão BOOTSEL**:
   - O código configura uma interrupção para o botão B (GPIO 6), permitindo reiniciar o dispositivo utilizando a funcionalidade BOOTSEL do RP2040.

## Configuração de Pinos

- **GPIO 14 (SDA)** e **GPIO 15 (SCL)** para comunicação I2C com o display OLED.
- **GPIO 28** para leitura do ADC (resistência desconhecida).
- **GPIO 6** para o botão BOOTSEL (reinicialização via BOOTSEL).
- **GPIO 7** para controle da matriz de LEDs WS2812.

## Como Usar

1. **Compilação e Upload**:
   - Compile o código usando o SDK do RP2040.
   - Faça o upload do código para a placa BitDogLab utilizando um adaptador USB para o RP2040.

2. **Uso do Ohmímetro**:
   - O valor da resistência será medido automaticamente assim que o código estiver rodando. O valor da resistência será exibido no display OLED.
   - As cores do código de cores do resistor (E24) serão exibidas tanto no display quanto na matriz de LEDs.

3. **Reinicialização**:
   - Pressione o **botão B** para reiniciar o sistema utilizando a funcionalidade BOOTSEL.

## Dependências

- **pico-sdk**: SDK do RP2040 para desenvolvimento no microcontrolador.
- **ssd1306**: Biblioteca para controle do display OLED SSD1306.
- **ws2812**: Biblioteca para controle da matriz de LEDs WS2812.

