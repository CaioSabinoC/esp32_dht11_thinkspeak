# Monitoramento de Parreiral - ESP32-C3 + DHT11

Projeto da disciplina de IoT: dispositivo simples para monitoramento de plantações em Petrolina, com foco em vinícolas, visando auxiliar na previsão de safras de frutas. O dispositivo lê temperatura e umidade com um sensor DHT11 e envia os dados periodicamente para o ThingSpeak.

## Equipe

- Arthur Vinicius
- Caio Sabino
- Pedro Rodrigues
- Marcos Vinicius
- Thauan Bezerra

## Hardware

- Placa ESP32-C3 (variante "SuperMini", com USB-Serial/JTAG nativo, 4MB de flash)
- Sensor DHT11 (módulo de 3 pinos com resistor de pull-up embutido)

## Ligação

| DHT11 | ESP32-C3 |
|-------|----------|
| VCC   | 3.3V     |
| GND   | GND      |
| DATA  | GPIO4    |

Se estiver usando o sensor DHT11 "pelado" (4 pinos, sem módulo), é necessário um resistor de 10kΩ entre VCC e DATA.

## Configuração do ambiente

Este projeto usa [PlatformIO](https://platformio.org/) com framework Arduino.

No `src/main.cpp`, preencha as constantes no topo do arquivo com os dados reais antes de compilar:

```cpp
const char* ssid = "SUA_REDE_AQUI";
const char* password = "SUA_SENHA_AQUI";
const char* thingSpeakApiKey = "SUA_WRITE_API_KEY_AQUI";
```

A Write API Key fica disponível em API Keys, nas configurações do seu canal no ThingSpeak.

**Atenção:** não faça commit do arquivo com suas credenciais reais preenchidas se o repositório for público — substitua de volta pelos placeholders antes de subir alterações.

## Compilar e gravar

```
pio run --target upload
pio device monitor
```

## Problema conhecido: falha de conexão Wi-Fi nesta placa

Durante o desenvolvimento, essa placa (ESP32-C3 "SuperMini") não conseguia conectar em nenhuma rede Wi-Fi, em nenhuma combinação testada: Arduino IDE e MicroPython, diferentes fontes de energia (USB do PC e fonte de parede), diferentes redes (WPA2, hotspot, rede aberta sem senha) e diferentes computadores. O status sempre voltava como desconectado, mesmo com a rede visível no scan e credenciais corretas.

O diagnóstico usando MicroPython foi o que revelou a pista real: em vez do erro genérico de "desconectado", ele retornou códigos de status específicos vindos do próprio roteador (como "senha incorreta"), o que provou que o rádio da placa conseguia transmitir e negociar com o ponto de acesso — descartando a hipótese de defeito físico na placa.

A causa raiz era a potência de transmissão do Wi-Fi no padrão de fábrica (máxima). Nessa placa clone, a potência máxima aparentemente distorce o sinal o suficiente para corromper os frames de autenticação, impedindo o handshake de completar. A solução foi reduzir a potência de transmissão logo após inicializar o modo Wi-Fi:

```cpp
WiFi.mode(WIFI_STA);
WiFi.setTxPower(WIFI_POWER_8_5dBm); // resolve a falha de conexao nessa placa
WiFi.disconnect();
```

Se você estiver usando uma placa ESP32-C3 "SuperMini" (ou clone similar) e tiver o mesmo sintoma — conecta em nenhuma rede, mas o scan funciona normalmente — vale tentar essa mesma solução antes de suspeitar de senha, protocolo de segurança ou energia.

## Envio para o ThingSpeak

Os dados são enviados a cada 20 segundos (o plano gratuito do ThingSpeak exige um intervalo mínimo de 15 segundos entre atualizações):

- `field1`: temperatura (°C)
- `field2`: umidade (%)
