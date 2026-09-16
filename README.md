# Monitoramento de Parreiral - ESP32-C3 + DHT11

Projeto da disciplina de IoT: dispositivo simples para monitoramento de plantações em Petrolina, com foco em vinícolas, visando auxiliar na previsão de safras de frutas. O dispositivo lê temperatura e umidade com um sensor DHT11 e envia os dados periodicamente para o ThingSpeak.

## Equipe

- Arthur Vinicius
- Caio Sabino
- Marcos Vinicius
- Pedro Rodrigues
- Thauan Bezerra

## Canal de Telemetria

https://thingspeak.mathworks.com/channels/3493396

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

## Memorial Descritivo

### Aplicação no negócio

No cenário do Vale do São Francisco (Petrolina/Juazeiro), este nó sensor se destina a ambientes onde o controle de microclima é crítico para o padrão fitossanitário de frutas de exportação (uva de mesa, manga): packing houses no pós-colheita, câmaras de resfriamento rápido, e carretas refrigeradas durante o transporte logístico até o porto ou centro de distribuição. O objetivo é gerar a telemetria inicial (edge) que alimentará, futuramente, modelos preditivos de qualidade de safra na infraestrutura em nuvem do PI.

### O que o protótipo faz

O ciclo de operação do firmware é:

1. Ao ligar, conecta-se à rede Wi-Fi configurada (com a correção de potência de transmissão descrita acima).
2. A cada 20 segundos, realiza a leitura digital do sensor DHT11 (temperatura em °C e umidade relativa em %).
3. Monta uma requisição HTTP GET para a API REST do ThingSpeak, enviando os valores lidos como campos (`field1`, `field2`) de uma série temporal.
4. Caso a conexão Wi-Fi caia em algum momento, o firmware detecta e reconecta automaticamente antes do próximo ciclo, sem precisar de reinício manual.

### Limitações técnicas identificadas

**Sensor DHT11:** possui resolução baixa (passos inteiros de 1°C e 1% de umidade) e tempo de resposta lento (cerca de 1 leitura por segundo, com atraso de sensibilidade de vários segundos para estabilizar em mudanças bruscas). Para aplicações de exportação com normas mais rígidas de rastreabilidade térmica, essa precisão é insuficiente — sensores como o DHT22 ou SHT31 seriam mais adequados, com melhor resolução e faixa de operação. O DHT11 também tem comportamento não confiável sob condensação (comum em câmaras frias com alta umidade), podendo gerar leituras `NaN` ou instáveis — o firmware já trata essa falha de leitura, mas o sensor não é ideal para o ambiente-alvo final.

**Vulnerabilidades do Wi-Fi:** o alcance de Wi-Fi 2.4GHz é limitado dentro de galpões metálicos (packing houses) e nulo durante o trânsito logístico (carretas refrigeradas em movimento, fora do alcance de qualquer rede local). Além disso, o firmware atual mantém o rádio Wi-Fi sempre ativo, sem uso de Deep Sleep, o que é aceitável para um protótipo alimentado via USB, mas inviável para um nó autônomo a bateria em campo — um redesign para operação intermitente (Deep Sleep entre leituras) ou migração para um protocolo de longo alcance e baixo consumo, como LoRaWAN, seria necessário para o cenário de transporte logístico.

**Limitações do invólucro reaproveitado:** a embalagem de upcycling utilizada não possui grau de proteção IP certificado. Isso a torna adequada apenas para ambientes controlados e sem exposição direta a respingos, poeira fina ou condensação superficial — não substitui um gabinete industrial com vedação (IP65 ou superior) exigido para uso permanente em câmaras frias ou packing houses reais.

## Próximos Passos (Integração com Cloud do PI)

O ThingSpeak funciona hoje como o ponto de ingestão inicial (camada edge) dos dados. Para a infraestrutura completa do PI, o fluxo evoluiria da seguinte forma: um serviço de integração (via API REST do ThingSpeak, que expõe os dados em JSON/CSV) consultaria periodicamente o canal e replicaria as leituras para um banco de dados de séries temporais em nuvem (por exemplo, AWS IoT Core + Timestream, ou Azure IoT Hub + Time Series Insights). A partir desse banco centralizado, seria possível: (1) agregar dados de múltiplos nós sensores distribuídos pelos diferentes pontos da cadeia (packing house, câmara fria, transporte); (2) alimentar modelos preditivos de qualidade de safra e vida útil pós-colheita, cruzando as séries de temperatura/umidade com dados históricos de perdas; e (3) gerar alertas automáticos em tempo real quando as variáveis saírem da faixa segura para o produto transportado.
