/*

  JUCA - Relé temporizador com Arduino, Display 1637 e Encoder Rotativo
  Versão 1.0.0

  Conexões:
  Encoder Rotativo: CLK -> 3, DT -> 4, Botão -> 5
  Display TM1637 : CLK -> 6, DIO -> 7
  Relé: 2

  Criado em 10/2021 por José Cintra
  https://josecintra.com/blog

  Este é um software gratuito e livre lançado em domínio público.
  Qualquer pessoa é livre para copiar, modificar, publicar, usar, compilar, vender ou
  distribuir este software, seja na forma de código-fonte ou como um arquivo
  binário, para qualquer finalidade, comercial ou não comercial, e por qualquer
  meios.

  Em jurisdições que reconhecem as leis de direitos autorais, o autor ou autores
  deste software dedicam todo e qualquer direito autoral no
  software para o domínio público. Fazemos essa dedicação pelo benefício
  do público em geral e em detrimento de nossos herdeiros e
  sucessores. Pretendemos que essa dedicação seja um ato aberto de
  renúncia perpétua de todos os direitos presentes e futuros a este
  software sob a lei de direitos autorais.

  O SOFTWARE É FORNECIDO "COMO ESTÁ", SEM QUALQUER TIPO DE GARANTIA,
  EXPRESSA OU IMPLÍCITA, INCLUINDO, MAS NÃO SE LIMITANDO ÀS GARANTIAS DE
  COMERCIALIZAÇÃO, ADEQUAÇÃO A UMA FINALIDADE ESPECÍFICA E NÃO VIOLAÇÃO.
  EM NENHUMA HIPÓTESE OS AUTORES SERÃO RESPONSÁVEIS POR QUALQUER RECLAMAÇÃO, DANOS OU
  OUTRAS RESPONSABILIDADES, SEJA EM AÇÃO DE CONTRATO, DELITO OU DE OUTRA FORMA,
  DECORRENTE DE, FORA DE OU EM CONEXÃO COM O SOFTWARE OU O USO OU
  OUTRAS NEGOCIAÇÕES NO SOFTWARE.

  Para obter mais informações, consulte <http://unlicense.org>

*/

/* Bibliotecas
 **************/
#include <SimpleRotary.h>      // Encoder - Disponível no gerenciador de bibliotecas do Arduino. Autor: MPrograms
#include <TM1637TinyDisplay.h> // Display - Disponível no gerenciador de bibliotecas do Arduino. Autor: Jason Cox
#include <neotimer.h>          // Millis  - Disponível no gerenciador de bibliotecas do Arduino. Autor: Jose Rullan
#include <EEPROM.h>            // EEPROM  - Disponível https://github.com/PaulStoffregen/EEPROM. Autor: Paul Stoffregen
SimpleRotary rotary(3, 4, 5);
TM1637TinyDisplay display(6, 7);
Neotimer contador = Neotimer(1000); // 1 Minuto

/* Variáveis globais
*********************/
int8_t tempos[3] = {0, 1, 0}; // Tempos escolhidos para cada contagem
int8_t repeticoes = 1;        // Número de repetições da programação
int8_t programa = 1;          // Programação atual
int8_t statusContagem = 0;    // 0 = Contagem parada | 1 = Primeira Contagem | 2 = Segunda contagem
int8_t m = 0;                 // Minuto atual na contagem
int8_t s = 0;                 // Segundo atual na contagem
int8_t r = 1;                 // Variável para controle das repetições de contagens

/* Constantes
**************/
const int8_t PINO_RELE  = 2;               // Pino onde está conectado o relé
const int8_t LIGA = HIGH;                  // Sinal para ligar o relé
const int8_t DESLIGA = LOW;                // Sinal para desligar o relé
const int8_t STOP[] PROGMEM = "JUCA";      // Texto para STOP
const int8_t SET[] PROGMEM = "OPC ";       // Texto para SET
const int8_t JUCA[] PROGMEM = "JUCA JUCA"; // Texto para JUCA

/* Setup
 ********/
void setup() {
  inicializaDisplay();
  pinMode(PINO_RELE, OUTPUT);
  finalizaContagem(true);
  leEEPROM();
}

/* Loop
 ********/
void loop() {

  // Leitura do botão do encoder
  int8_t p = rotary.pushType(2000);

  // Botão pressionado com toque curto inicia/finaliza contador
  if (p == 1) {
    if (statusContagem == 0) { // Com o contador parado, iniciar contador com o útimo tempo escolhido
      statusContagem = 1;
      iniciaContagem();
    } else { // Com o contador em andamento, finaliza o contador
      finalizaContagem(true);
    }
  }

  // Botão pressionado com toque longo. Entra em modo de configuração
  if (p == 2) {
    finalizaContagem(false);
    mudaConfiguracao();
    finalizaContagem(true);
  }

  // Exibição da contagem
  if ((statusContagem != 0) && contador.done()) {
    s--;
    if (s < 0) {
      s = 59;
      m--;
    }
    if (m < 0) {
      m = 0;
      s = 0;
    }
    exibeContagem();
    contador.start();
  }

  // Controle programação
  if (m == 0 && s == 0) {
    if (statusContagem == 1) {
      statusContagem = 2;
      iniciaContagem();
    } else if (statusContagem == 2) {
      r = (repeticoes == 0) ? 0 : (r + 1);
      if (r > repeticoes) {
        finalizaContagem(true);
      } else {
        statusContagem = 1;
        iniciaContagem();
      }
    }
  }

}

/* Exibe o contador
****************/
void exibeContagem() {
  display.showNumberDec(m, 0b01000000, true, 2, 0);
  display.showNumberDec(s, 0b01000000, true, 2, 2);
}

/* Exibe um texto no display e, opcionalmente, aguarda um tempo
****************************************************************/
void exibeTexto(const int8_t *txt PROGMEM, int espera) {
  limpaDisplay();
  display.showString_P(txt);
  if (espera != 0) {
    delay(espera);
  }
}

/* Limpa o display
*******************/
void limpaDisplay() {
  display.clear();
}

/* Inicializa o display
************************/
void inicializaDisplay() {
  display.setBrightness(BRIGHT_4);
  display.setScrolldelay(300);
  exibeTexto(JUCA, 0);
}

/* Inicia o contador
*********************/
void iniciaContagem() {
  contador.stop();
  if (statusContagem > 2) {
    statusContagem = 2;
  }
  // Na linha abaixo, aplicamos uma operação XAND (Not XOR) nas entrada para obeter o estado do relé
  digitalWrite(PINO_RELE, !((programa - 1) ^ (statusContagem - 1)));
  m = tempos[(int) statusContagem];
  s = 0;
  exibeContagem();
  contador.start();
}

/* Finaliza o contador
********************/
void finalizaContagem(bool exibeStop) {
  digitalWrite(PINO_RELE, DESLIGA);
  contador.stop();
  statusContagem = 0;
  r = 1;
  if (exibeStop) {
    exibeTexto(STOP, 1000);
  }
}

/* Modo de configuração - Menu
*******************************/
void mudaConfiguracao() {
  exibeTexto(SET, 1000);
  String parNome[4] = {"1P", "2P", "3P", "4P"}; // Tempo ligado/ Tempo desligador / Repetições / Programa
  int8_t parMin[4]  = {0, 0, 0, 1};
  int8_t parMax[4]  = {60, 60, 99, 2};
  int8_t parVal[4]  = {tempos[1], tempos[2], repeticoes, programa};
  int8_t r, p; // Rotação e PushButton
  long pisca = -1;
  int numPar = 0;
  while (numPar < 4) {
    r = rotary.rotate();
    p = rotary.push();
    while ( (r == 1) || (r == 2) ) { // Direita ou esquerda
      if ((r == 1) && (parVal[numPar] < parMax[numPar])) {
        parVal[numPar]++;
      }
      if ((r == 2) && (parVal[numPar] > parMin[numPar])) {
        parVal[numPar]--;
      }
      r = rotary.rotate();
      exibeParametro(parNome[numPar], parVal[numPar]);
      pisca = -1;
    }
    if ( p == 1 ) { // Push
      numPar++;
      if (numPar < 4) {
        exibeParametro(parNome[numPar], parVal[numPar]);
      }
    }
    // Efeito piscante
    pisca++;
    if (pisca == 0 && numPar < 4) {
      exibeParametro(parNome[numPar], parVal[numPar]);
    }
    if (pisca == 50000) {
      limpaDisplay();
    }
    if (pisca >= 75000) {
      pisca = -1;
    }
  }
  // Ajuste dos parâmetros de configuração e gravação na eeprom
  // os valores nos endereços 0 e 5 são para controle
  if (parVal[4] = 1) {
    EEPROM.write(1, parVal[0]);
    EEPROM.write(2, parVal[1]);
  } else {
    EEPROM.write(1, parVal[1]);
    EEPROM.write(2, parVal[0]);
  }
  EEPROM.write(3, parVal[2]);
  EEPROM.write(4, parVal[3]);
  EEPROM.write(0, 32);
  EEPROM.write(5, 32);
  leEEPROM();
}

void exibeParametro(String nome, int8_t val) {
  limpaDisplay();
  String auxStr = nome + ((val < 10) ? "0" + String(val) : String(val));
  char auxChar[5];
  auxStr.toCharArray(auxChar, 5);
  display.showString(auxChar);
}

void leEEPROM() {
  if (EEPROM.read(0) == 32 && EEPROM.read(5) == 32 ) {
    tempos[1] = EEPROM.read(1);
    tempos[2] = EEPROM.read(2);
    repeticoes = EEPROM.read(3);
    programa = EEPROM.read(4);
  }
}
