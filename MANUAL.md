# ZEMUN — manual

**betobullet** · 14 HP · filtro multimodo de dois picos

O Zemun é um filtro de variáveis de estado duplo: dois núcleos ressonantes combinados em série ou
em paralelo, com dez respostas. Ele se baseia no comportamento publicado do Xaoc Belgrad; o
processamento é próprio, com dois filtros não lineares e oversampling de 2x.

---

## 1. Controles

| Controle | Faixa | O que faz |
|---|---|---|
| **FREQ** | 4 Hz – 28 kHz | Frequência central dos dois picos. |
| **LEVEL** | 0–100 % | Ganho de entrada. Acima de cerca de 40 % começa a saturar suavemente. |
| **SPAN** | 0–8 oitavas | Distância entre os dois picos, com curva não linear. Em 0 os picos coincidem. |
| **RESO** | 0–10 | Ressonância. Por volta de 8 entra em auto-oscilação afinada; acima disso distorce. |
| **BALANCE** | grave ↔ agudo | Desloca a ressonância entre o pico grave e o agudo. |
| **MODE** | 10 posições | Tipo de resposta (tabela abaixo). |
| **TITO** | SM / off / XM | Modulação de áudio entre os núcleos: SM = cada núcleo modula a si mesmo; XM = cruzada. |
| **Slider FM** | 0–100 % | Profundidade do CV em FM. O slider acende com o CV. |
| **Slider SPAN** | 0–100 % | Profundidade do CV de SPAN. Também acende com o CV. |

## 2. Modos

| Modo | Resposta |
|---|---|
| **LL** | passa-baixas de dupla inclinação (dois LP em série) |
| **LB** | passa-baixas + passa-banda |
| **NL** | passa-baixas com notch |
| **NN** | notch duplo |
| **LH** | passa-baixas + passa-altas (rejeita banda entre os picos) |
| **BB** | passa-banda duplo (dois picos) |
| **HL** | passa-banda (HP seguido de LP) |
| **HN** | passa-altas com notch |
| **BH** | passa-banda + passa-altas |
| **HH** | passa-altas de dupla inclinação |

## 3. Entradas e saída

| Jack | Faixa |
|---|---|
| **INPUT** | áudio, ±5 V nominal |
| **V/OCT** | 1 V/oitava na frequência (os dois picos acompanham) |
| **FM** | FM exponencial, escalada pelo slider FM |
| **SPAN** | ±5 V cobrem as 8 oitavas com o slider no máximo |
| **RESO** | 0–8 V percorrem o knob inteiro |
| **BAL** | ±5 V percorrem o balance inteiro |
| **OUTPUT** | saída do filtro |

As entradas são **polifônicas**: o filtro roda uma voz por canal de INPUT ou de V/OCT.

## 4. Dicas

- **Oscilador senoidal:** sem entrada, com RESO acima de 8 e V/OCT ligado, o filtro vira um
  oscilador afinado.
- **Vogais e formantes:** use BB com SPAN médio e RESO alta, e varie o SPAN por CV.
- **Caos controlado:** TITO em XM com RESO perto de 8.

## 5. Menu de contexto

- **Theme**: Black ou White.
