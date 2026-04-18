# Projeto Final — TinyML Embarcado com Raspberry Pi Pico W, MPU6050 e Wokwi

## 1. Visão geral do projeto

Este projeto teve como objetivo embarcar um modelo **TinyML** treinado no [**Edge Impulse**](https://studio.edgeimpulse.com/studio/961956) em um firmware para **Raspberry Pi Pico W**, utilizando um **MPU6050** como fonte de dados e a plataforma [**Wokwi**](https://wokwi.com/projects/461230442260500481) para simulação.

A proposta prática foi classificar o comportamento de uma máquina em duas classes:

- **OK**
- **falha**

O modelo foi treinado a partir de dados de acelerômetro e, em seguida, integrado ao código-fonte do dispositivo para executar inferência localmente com dados simulados em tempo real.

---

## 2. Conceitos utilizados

### TinyML
TinyML é a aplicação de modelos de Machine Learning em dispositivos embarcados com recursos limitados de processamento e memória. Neste projeto, o modelo foi exportado pelo Edge Impulse em formato de **biblioteca C/C++**, permitindo a execução local da inferência no firmware.

### Edge Impulse
O Edge Impulse foi utilizado para:

- coletar e organizar os dados do acelerômetro
- treinar o modelo de classificação
- exportar a biblioteca embarcável em C/C++

### Inferência embarcada
A inferência embarcada consiste em executar o modelo diretamente no microcontrolador, sem dependência de nuvem. Neste projeto, o fluxo ficou assim:

1. leitura dos eixos `ax`, `ay`, `az`
2. armazenamento das amostras em uma janela temporal
3. chamada da função de inferência
4. retorno da classe predita e da confiança

### Simulação no Wokwi
O Wokwi foi utilizado para simular:

- a placa **Raspberry Pi Pico W**
- o sensor **MPU6050**
- o display **SSD1306**

Isso permitiu validar o modelo com dados controlados antes do uso em hardware físico.

---

## 3. Hardware e conexões utilizadas

## Placa principal
- **Raspberry Pi Pico W**

## Sensor
- **MPU6050**

## Display
- **SSD1306 OLED 128x64**

## Barramentos I2C utilizados
O projeto utilizou **dois barramentos I2C independentes**:

### I2C0 — MPU6050
- `GP4` → `SDA`
- `GP5` → `SCL`
- `3V3` → `VCC`
- `GND` → `GND`
- endereço I2C do sensor: `0x68`

### I2C1 — SSD1306
- `GP14` → `SDA`
- `GP15` → `SCL`
- `3V3` → `VCC`
- `GND` → `GND`
- endereço I2C do display: `0x3C`

## Motivo da separação em dois barramentos
A utilização de dois barramentos I2C simplificou a organização do projeto, evitando conflitos entre sensor e display e deixando a estrutura mais clara para depuração e manutenção.

---

## 4. Base do firmware e espelhamento com o projeto blink

Para viabilizar a compilação local no **VS Code + Pico SDK**, a base do projeto foi construída a partir de um projeto funcional gerado pelo exemplo **`blink`**.

O procedimento adotado foi:

1. criar e compilar o projeto `blink`
2. duplicar essa base
3. substituir o conteúdo do exemplo por `main.c`
4. adaptar o `CMakeLists.txt`
5. manter a estrutura reconhecida pela extensão oficial da Raspberry Pi Pico

Esse espelhamento foi importante porque permitiu:

- validar o ambiente local antes de integrar o modelo
- herdar uma estrutura de projeto compatível com o Pico SDK
- evitar iniciar a integração do TinyML em um ambiente ainda instável

Em outras palavras, o projeto final nasceu sobre uma base mínima já comprovadamente compilável.

---

## 5. Estrutura lógica do sistema

A solução final foi organizada em três camadas:

### Camada 1 — Aquisição e interface embarcada (`main.c`)
Responsável por:

- inicialização dos barramentos I2C
- leitura do MPU6050
- conversão dos valores para `g`
- renderização das informações no OLED
- exibição das leituras no Serial Monitor

### Camada 2 — Ponte entre C e C++ (`ei_bridge.h` / `ei_bridge.cpp`)
Responsável por:

- receber amostras do `main.c`
- preencher a janela de inferência
- chamar o `run_classifier()`
- devolver a classe predita e a confiança

Essa ponte foi necessária porque o firmware principal estava em **C**, enquanto a biblioteca exportada pelo Edge Impulse foi gerada em **C++**.

### Camada 3 — Biblioteca Edge Impulse
Responsável por:

- DSP
- pipeline de inferência
- TensorFlow Lite Micro
- modelo treinado

---

## 6. Parâmetros do modelo

O modelo final foi retreinado para ficar coerente com o firmware e com a simulação.

## Entradas
- `ax`
- `ay`
- `az`

## Unidade
- dados em **g**

## Janela temporal
- **2000 ms**

## Taxa de amostragem prática utilizada no firmware
- **1 amostra a cada 100 ms**
- equivalente a **10 Hz**

## Quantidade de amostras por janela
- **20 amostras**

Isso permitiu que a coleta do `main.c` fosse compatível com o treinamento do modelo.

---

## 7. Funcionamento da inferência

O funcionamento da inferência no firmware seguiu a lógica abaixo:

1. o código lê `ax`, `ay` e `az`
2. cada trio de valores é enviado para a ponte `ei_bridge`
3. após completar a janela de 20 amostras, o modelo é executado
4. a classe mais provável é selecionada
5. o resultado é exibido no **Serial Monitor** e no **OLED**

A saída final exibida foi composta por:

- classe vencedora
- score percentual aproximado

Exemplo:

```text
PRED: OK (100)
PRED: falha (98)
```

No OLED, foi adicionada uma linha com a classe e a confiança da última inferência.

---

## 8. Resultados observados

Os testes realizados no Wokwi mostraram que:

- em condição estável, o modelo convergiu para **OK**
- após alteração dos valores do MPU6050 simulado, o modelo passou a classificar como **falha**
- em regiões intermediárias, os scores variaram, indicando transição de confiança entre as classes

Esse comportamento mostra que o sistema não ficou travado em uma única resposta. Pelo contrário, a inferência reagiu à mudança do padrão de entrada, o que caracteriza uma integração funcional entre:

- coleta de dados
- janela temporal
- modelo treinado
- inferência embarcada
- simulação em tempo real

---

## 9. Principais desafios técnicos

Durante o desenvolvimento, os principais desafios foram:

### 1. Ambiente local no Windows ARM
O projeto foi desenvolvido em um **Surface Pro 11 com Windows on ARM**, o que exigiu ajuste da extensão da Raspberry Pi Pico para a versão **pre-release** antes de o ambiente passar a compilar corretamente.

### 2. Integração C com biblioteca C++
O firmware estava em **C**, enquanto a biblioteca do Edge Impulse foi exportada em **C++**. Isso exigiu a criação de uma ponte dedicada.

### 3. Compatibilização do build com Pico SDK
A biblioteca exportada não estava pronta para uso direto no formato do projeto Pico SDK, exigindo adaptação do `CMakeLists.txt`.

### 4. Ajustes de dependências de inferência
Foi necessário incluir corretamente:

- porting da biblioteca
- componentes do TensorFlow Lite Micro
- partes do CMSIS-DSP
- configurações de compilação compatíveis com o RP2040

### 5. Coerência entre treino e aquisição
O modelo só passou a funcionar corretamente depois que os parâmetros de treino foram alinhados com o comportamento real do firmware.

---

## 10. Entregáveis gerados

Os entregáveis finais do projeto são:

- código-fonte do projeto em repositório Git
- [vídeo da simulação local no Wokwi](https://drive.google.com/file/d/1gsYdyLHdexhCfx3g63-Bhz-SnRCF-3ow/view?usp=sharing)
- documentação técnica deste projeto

---

## 11. Conclusão

O projeto demonstrou com sucesso a aplicação de **TinyML embarcado** em um sistema simulado com **Raspberry Pi Pico W**, **MPU6050** e **SSD1306**, utilizando **Edge Impulse** para treinamento e **Wokwi** para validação.

O resultado final comprova que foi possível:

- capturar dados do acelerômetro
- formar janelas temporais
- executar inferência local
- classificar o comportamento em tempo real
- visualizar o resultado no serial e no display

Portanto, o objetivo do trabalho foi atingido: o modelo embarcado ficou operacional e reagiu de forma coerente às mudanças dos dados de entrada durante a simulação.
