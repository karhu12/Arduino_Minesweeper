/* Implementoitavia ominaisuuksia tai muita ideoita 
 * Mahdollisesti RGB ledi matriisin käyttö
 * Ehkä kaiutin ja ääniefektejä / musiikkia peliin
 * Lisää valikon ominaisuuksia
 */

/* Arduino Miinaharava ohjelma
 * Tekijät : Riku K, Janne H. Juho P.
 * V 0.19
 */

//LIBRARIES
#include "LedControl.h"
#include "LiquidCrystal.h"

//GLOBAL DEFINITIONS
#define MAX_WIDTH 8
#define MAX_HEIGHT 8

enum SELECTIONS 
{EASY_DIFFICULTY = 0,MEDIUM_DIFFICULTY = 1,HARD_DIFFICULTY = 2,
BRUTAL_DIFFICULTY = 3,BUTTONPRESSED = 2,JOYSTICKMOVED = 1};

enum POSITIONS
{BOMB = 99, UNCHECKED = 0,NO_BOMBS = -1};

//GLOBALS
LedControl lc = LedControl(13,11,12,1);
const int rs = 2;
const int enable = 3;
const int d4 = 4, d5 = 5, d6 = 6, d7 = 7;
LiquidCrystal lcd(rs, enable, d4,  d5, d6, d7);
int bombAmount;
int victory;
const int inX = A0; //A1 pinniin yhdistetty joystickX
const int inY = A1; //A0 pinniin yhdistetty joystickY
const int inSwitch = 8; //A2 pinniin yhdistetty nappi
const int speakerPin = 9;
int ledPosState = 0;
int wonGameArt[8][8] = {
  {0,0,1,1,1,1,0,0},
  {0,1,0,0,0,0,1,0},
  {1,0,1,0,1,0,0,1},
  {1,0,0,0,0,1,0,1},
  {1,0,0,0,0,1,0,1},
  {1,0,1,0,1,0,0,1},
  {0,1,0,0,0,0,1,0},
  {0,0,1,1,1,1,0,0}
};


//FUNCTION DECLARATION
int ifInput();
void printBombValues(int bombX[],int bombY[]);
void printPosValues(int posx,int posy);
void blinkPos(int posx, int posy,int ms);
void wonGame();
void accBlinkPos(int posx,int posy);
void lostGame(int posx,int posy,int pos_status[][MAX_WIDTH]);
int movePosX(int axisX,int posx);
int movePosY(int axisY,int posy);
void generateBombs(int bombX[], int bombY[], int pos_status[][MAX_WIDTH]);
void checkPosBombs(int posx,int posy,int bombX[],int bombY[],int *game,int pos_status[][MAX_WIDTH]);
void setledPosStates(int pos_status[][MAX_WIDTH],int posx,int posy,int *interval);
void displayLcd(int pos_status[][MAX_WIDTH],int posx,int posy,int input);
int prepareLedState(unsigned long currentMillis,unsigned long *previousMillis,int interval);
void checkSurroundingMines(int posx,int posy,int bombX[],int bombY[],int pos_status[][MAX_WIDTH]);
int mainMenu();
void selectedDifficulty(int difficulty);
void playTone(char *note,int duration);

//ARDUINO SETUP
void setup() {
  Serial.begin(9600);
  pinMode(inX,INPUT);
  pinMode(inY,INPUT);
  pinMode(inSwitch,INPUT_PULLUP);
  pinMode(speakerPin,OUTPUT);
  randomSeed(analogRead(A5));  //Lukee satunnaisarvon siemenen analogisesta pinnistä (häiriö)
  lc.shutdown(0,false);
  lc.setIntensity(0,2);
  lc.clearDisplay(0);
  lcd.begin(16, 2);
}


//MAIN LOOP
void loop() {

 //MENU
  int difficulty = mainMenu();
  selectedDifficulty(difficulty);
  
  //GAME VARIABLES
  int interval = 100;
  unsigned long previousMillis = 0;
  //Pelaajan aloitus sijainti on kentän keskellä
  int posx = MAX_WIDTH/2, posy = MAX_HEIGHT/2;
  int game = 1;
  int input = 0;
  float axisX,axisY;
  int bombX[bombAmount], bombY[bombAmount];
  //Pelikenttä alustetaan 0 arvolla tarkistamattoman merkiksi
  int pos_status[MAX_HEIGHT][MAX_WIDTH];
  for (int i = 0; i < MAX_HEIGHT; i++) {
    for (int j = 0; j < MAX_WIDTH; j++) {
      pos_status[i][j] = 0;
    }
  }

  //GAME SETUP
  lcd.clear();
  generateBombs(bombX,bombY,pos_status);
  printBombValues(bombX,bombY);
  printPosValues(posx,posy);
  
  //GAME LOOP
  while (game) {
    unsigned long currentMillis = millis();
    setledPosStates(pos_status,posx,posy,&interval);
    input = ifInput();
    // Ohjaussauvan tilasta riippuen siirretään pelaajaa
    if (input == JOYSTICKMOVED) {
      axisX = analogRead(inX);
      axisY = analogRead(inY);
      posx = movePosX(axisX,posx);
      posy = movePosY(axisY,posy);
      printPosValues(posx,posy);
      displayLcd(pos_status,posx,posy,input);
    }
    // Napin painalluksesta tarkistetaan sijainti pommeista ja viereisistä pommeista
    else if (input == BUTTONPRESSED) {
      if (pos_status[posy][posx] == UNCHECKED || pos_status[posy][posx] == BOMB) {
        tone(speakerPin,6000,150);
        checkPosBombs(posx,posy,bombX,bombY,&game,pos_status);
        if (game == 1) {
          checkSurroundingMines(posx,posy,bombX,bombY,pos_status);
          accBlinkPos(posx,posy);
        }
        displayLcd(pos_status,posx,posy,input);
        delay(1000);
      }
      else {
        ledPosState = prepareLedState(currentMillis,&previousMillis,interval);
      }
    }
    // kun ei ole käyttäjän painallusta vaihdetaan ledin tilaa vilkkumista varten
    else {
      displayLcd(pos_status,posx,posy,input); 
      ledPosState = prepareLedState(currentMillis,&previousMillis,interval);
    }
    // Lopussa tarkistetaan onko peli voitettu
    if (victory == 0) {
      wonGame();
      game = 0;
    }
  }
}


//FUNCTIONS

// Funkio joka palauttaa 1 jos ohjaussauva oli käännetty, 2 jos nappi on painettu tai 0 ellei kumpikaan ole aktiivinen. 
int ifInput() {
  int input = 0;
  if ( (analogRead(inY) > 800 ) || (analogRead(inY) < 200 ) || (analogRead(inX) > 800 ) || (analogRead(inX) < 200) ) {
    Serial.println("Input found");
    input = 1;
  }
  if (digitalRead(inSwitch) == LOW) {
    Serial.println("Button pressed");
    input = 2;
  }
  return input;
}

// Tulostaa pommi taulukkojen arvot
void printBombValues(int bombX[],int bombY[]) {
  for (int i = 0; i < bombAmount; i++) {
    Serial.print("Bomb ");
    Serial.print(i+1);
    Serial.print(" value x : ");
    Serial.print(bombX[i]);
    Serial.print(" Y : ");
    Serial.println(bombY[i]);
  }
}

// Tulostaa pelaajan sijainnin
void printPosValues(int posx,int posy){
  Serial.print("Position x value : ");
  Serial.println(posx);
  Serial.print("Position y value : ");
  Serial.println(posy);
}

// Välkyttää lediä pelaajan sijainnissa
void blinkPos(int posx, int posy,int ms) {
  lc.setLed(0,(MAX_WIDTH-1)-posx,posy,true);
  delay(ms);
  lc.setLed(0,(MAX_WIDTH-1)-posx,posy,false);
  delay(ms);
}

void wonGame() {
  delay(100);
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("You won!");
  lcd.setCursor(0,1);
  lcd.print("Congratulations!");
  playTone('B',214);
  playTone('B',214);
  playTone('B',214);
  playTone('B',428);
  playTone('G',428);
  playTone('A',540);
  playTone('B',428);
  playTone('G',214);
  playTone('B',800);
  playTone('F',428);
  playTone('E',428);
  playTone('F',428);
  playTone('E',214);
  playTone('A',214);
  playTone('A',214);
  playTone('A',214);
  playTone('G',428);
  playTone('A',214);
  playTone('G',428);
  playTone('G',214);
  playTone('F',428);
  playTone('E',428);
  playTone('e',428);
  playTone('E',214);
  playTone('d',428);
  playTone('d',428);
  
  for (int x = 0; x < 3; x++) {
    delay(500);
    for (int i = 0; i < MAX_HEIGHT; i++) {
      for (int j = 0; j < MAX_WIDTH; j++) {
        lc.setLed(0,(MAX_WIDTH-1)-j,i,false);
      }
    }
    delay(500);
    for (int i = 0; i < MAX_HEIGHT; i++) {
      for (int j = 0; j < MAX_WIDTH; j++) {
        if (wonGameArt[i][j] == 1) lc.setLed(0,(MAX_WIDTH-1)-j,i,true);
        else lc.setLed(0,(MAX_WIDTH-1)-j,i,false);
      }
    }  
  }
  delay(1000);
  for (int i = 0; i < MAX_HEIGHT; i++) {
    for (int j = 0; j < MAX_WIDTH; j++) {
      lc.setLed(0,(MAX_WIDTH-1)-j,i,true);
      delay(20);
    }
  }
}

// Välkyttää pelinhäviön merkiksi ledejä, mutta ensin tekee saman kiihtyvän välkkymisen valinnan merkiksi
void lostGame(int posx,int posy,int pos_status[][MAX_WIDTH]) {
  accBlinkPos(posx,posy);
  delay(100);
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("You lost!");
  lcd.setCursor(0,1);
  lcd.print("You hit a mine");
  for (int i = 0; i < MAX_HEIGHT; i++) {
    for (int j = 0; j < MAX_WIDTH; j++) {
      lc.setLed(0,(MAX_WIDTH-1)-j,i,false);
    }
  }
  for (int i = 0; i < 3; i++) {
    for(int i = 0; i < MAX_HEIGHT; i++) {
      for (int j= 0; j < MAX_WIDTH; j++) {
        if (pos_status[i][j] == BOMB) {
          lc.setLed(0,(MAX_WIDTH-1)-j,i,true);
        }
        else {
          lc.setLed(0,(MAX_WIDTH-1)-j,i,false);
        }
      }
    }
    playTone('C',500);
    for(int i = 0; i < MAX_HEIGHT; i++) {
      for (int j= 0; j < MAX_WIDTH; j++) {
        lc.setLed(0,i,j,false);
      }
    }
    playTone('A',500);
  }
  delay(1000);
  for (int i = 0; i < MAX_HEIGHT; i++) {
    for (int j = 0; j < MAX_WIDTH; j++) {
      lc.setLed(0,(MAX_WIDTH-1)-j,i,true);
      delay(20);
    }
  }
}

// Kiihtyvä välkkyminen pelaajan sijainnissa
void accBlinkPos(int posx,int posy) {
  blinkPos(posx,posy,100);
  blinkPos(posx,posy,80);
  blinkPos(posx,posy,70);
  blinkPos(posx,posy,60);
  blinkPos(posx,posy,50);
  blinkPos(posx,posy,40);
  blinkPos(posx,posy,30);
  blinkPos(posx,posy,20);
  blinkPos(posx,posy,10);
}

// Siirtää pelaajaa x-akselilla oikealle tai vasemmalle riippuen tatin arvosta. Jos pelaaja on kentän rajalla siirtoa ei tehdä.
int movePosX(int axisX,int posx){
  //Siirto vasemmalle
  if (axisX > 800) {
    if (posx == 0) { 
      Serial.println("posX cant go < 0");
    }
    else {
      posx = posx - 1;
      tone(speakerPin,6000,50);
      while (analogRead(inX) > 800){}
      noTone(speakerPin);
    }
  }
  //Siirto oikealle
  if (axisX < 200) {
    if (posx == MAX_WIDTH-1) {
      Serial.println("posX cant go > MAX_WIDTH");
    }
    else { 
      posx = posx + 1;
      tone(speakerPin,6000,50);
      while (analogRead(inX) < 200){}
      noTone(speakerPin);
    }
  }
  return posx;  
}

// Siirtää pelaaja y-akselilla ylös tai alas samanlaisilla ehdoilla kuin aiemmassa funktiossa
int movePosY(int axisY,int posy) {
  // Siirto alas
  if (axisY > 800) {
    if (posy == MAX_HEIGHT-1) {
      Serial.println("posY cant go > MAX_HEIGHT");
    }
    else {
      posy += 1;
      tone(speakerPin,6000,50);
      while (analogRead(inY) > 800){}
      noTone(speakerPin);
    }
  }
  // Siirto ylös
  if (axisY < 200) {
    if (posy == 0) {
      Serial.println("posY cant go < value 0");
    }
    else {
      posy -= 1;
      tone(speakerPin,6000,50);
      while (analogRead(inY) < 200){}
      noTone(speakerPin);
    }
  }
  return posy;
}

// Funktio luo satunnaiset sijainnit pommeille ja sijoittaa ne taulukkoon.
void generateBombs(int bombX[], int bombY[], int pos_status[][MAX_WIDTH]) {
  for (int i = 0; i < bombAmount; i++) {
    for (int j = 0; j < bombAmount; j++) {
      bombY[i] = random(MAX_HEIGHT);
      bombX[j] = random(MAX_WIDTH);
    }
  }
  // Vertaillaan onko luodut luvut samoja ja toistetaan niin kauan kunnes jokainen pommi on omassa kohdassaan
  for (int y = 0; y < bombAmount; y++) {
    for (int x = 0; x < bombAmount; x++) {
      if (x == y) {
        break;
      }
      else if (bombX[y] == bombX[x] && bombY[y] == bombY[x]) {
        bombX[y] = random(MAX_HEIGHT);
        bombY[y] = random(MAX_WIDTH);
        x = -1;
        Serial.println("Bomb re-generated");
      }
    }
  }
  // Asetetaan pommien arvo 99:ksi pos_status taulukkoon muuhun käyttöön
  for (int i = 0; i < bombAmount; i++) {
    pos_status[bombY[i]][bombX[i]] = BOMB;
  }
}

// Tarkistaa onko pelaajan haravoimassa sijainnissa pommeja ja jos on niin peli hävitään
void checkPosBombs(int posx,int posy,int bombX[],int bombY[],int *game,int pos_status[][MAX_WIDTH]) {
  for (int i = 0; i < bombAmount; i++) {
    if (posx == bombX[i] && posy == bombY[i]) {
      *game = 0;
      Serial.println("YOU LOST");
      lostGame(posx,posy,pos_status);
      break;
    }
  }
}

// Sytyttää Led matriisin pos_status taulukon arvojen mukaan. 0/99 = HIGH, -1/>0 = LOW
void setledPosStates(int pos_status[][MAX_WIDTH],int posx,int posy,int *interval) {
  for (int i = 0; i < MAX_HEIGHT; i++) {
    for (int j = 0; j < MAX_WIDTH; j++) {
      if (pos_status[i][j] == UNCHECKED || pos_status[i][j] == BOMB) {
        if (i == posy && j == posx) {
          // Interval muuttaa jatkuvaa pelaajansijainnin vilkutus nopeutta
          *interval = 100;
          lc.setLed(0,(MAX_WIDTH-1)-j,i,ledPosState);
        }
        else {
          lc.setLed(0,(MAX_WIDTH-1)-j,i,true);
        }
      }
      else if ( ( pos_status[i][j] == NO_BOMBS ) || ( pos_status[i][j] > UNCHECKED && pos_status[i][j] < BOMB ) ) {
        if (i == posy && j == posx) {
            //Kun pelaaja on haravoidussa kohdassa vilkutus hidastuu merkiksi
            *interval = 400;
            lc.setLed(0,(MAX_WIDTH-1)-j,i,ledPosState);
        }
        else {
            lc.setLed(0,(MAX_WIDTH-1)-j,i,false);
        }
      }
    }   
  }
}

// Funktio jolla ohjataan LCD näyttöä pelin aikana
void displayLcd(int pos_status[][MAX_WIDTH],int posx,int posy,int input) {
  if (input == 1 || input == 2) lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Total mines:");
  lcd.print(bombAmount);
  lcd.setCursor(0,1);
  if (pos_status[posy][posx] > UNCHECKED && pos_status[posy][posx] != BOMB) {
    lcd.print("Mines nearby: ");
    lcd.print(pos_status[posy][posx]);
  }
  else if (pos_status[posy][posx] == UNCHECKED) {
    lcd.print("Not revealed");
  }
  else if (pos_status[posy][posx] == NO_BOMBS) {
    lcd.print("Mines nearby: 0");   
  }
  else {
    lcd.print("Not revealed");
  }
}

// Funktiolla ohjataan pelaajan sijainnissa olevan ledin vilkkumista 0/1 arvolla (0 LOW , 1 HIGH)
int prepareLedState(unsigned long currentMillis,unsigned long *previousMillis,int interval) {
  if (currentMillis - *previousMillis >= interval) {
    *previousMillis = currentMillis;
    if (ledPosState == 0) {
      return 1;
    } else {
      return 0;
    }
  }
}

// Funktio jolla tarkistetaan montako pommia haravoidun kohdan vieressä on. Pommien ollessa 0 funktio etsii ympäröiviä alueita pommeilta
void checkSurroundingMines(int posx,int posy,int bombX[],int bombY[],int pos_status[][MAX_WIDTH]) {
  int tempPosx = posx;
  int tempPosy = posy;
  // Toistolause joka tarkistaa viereiset koordinaatit pommeista ja palauttaa tarkistettuun kohtaan montako pommia vieressä oli
  for (int i = tempPosy-1; i < tempPosy+2; i++) { 
    for (int j = tempPosx-1; j < tempPosx+2; j++) {
      if ( j >= 0 && j <= 7 &&  i >= 0 && i <= 7 ) {
        if (pos_status[i][j] == UNCHECKED || pos_status[i][j] == BOMB) {
          for (int b = 0; b < bombAmount; b++) {
            if (i == bombY[b] && j == bombX[b]) {
              pos_status[tempPosy][tempPosx] += 1;
            }
          }
        }
      }
    }
  }
  // Jos kohdan vieressä oli pommeja niin päivitetään voittoehtoa yhdellä ja poistutaan funktiosta
  if (pos_status[tempPosy][tempPosx] > UNCHECKED && pos_status[tempPosy][tempPosx] != BOMB ) {
    victory -= 1;
  }
  // Ellei pommeja löytynyt niin päivitetään voitto ehto uudestaan ja asetetaan nykyinen kohta tarkistetuksi -1:llä
  else if (pos_status[tempPosy][tempPosx] == 0) {
    pos_status[tempPosy][tempPosx] -= 1;
    victory -= 1;
    // Rekursiivinen funktio joka toistuu niin kauan, että paljastettujen kohtien ympärillä on pommeja tai reuna
    for (int y = tempPosy-1; y < tempPosy+2; y++) {
      for (int x = tempPosx-1; x < tempPosx+2; x++) {
        if ( y >= 0 && y <= 7 &&  x >= 0 && x <= 7 ) {
          if (pos_status[y][x] == UNCHECKED) {
            checkSurroundingMines(x,y,bombX,bombY,pos_status);
          }
        }
      }
    }
  }
}

// Pelin vaikeustason valinta
int mainMenu() {
  lcd.clear();
  static int difficulty = 1;
  while (true) {
    if (JOYSTICKMOVED == ifInput()) {
      if (analogRead(inY) > 800) {
        if (difficulty != 0)
          difficulty -= 1;
        else
          difficulty = 3;
        while (analogRead(inY) > 800){}        
        lcd.clear();
      }
      else if (analogRead(inY) < 200) {
        if (difficulty != 3) 
          difficulty += 1;
        else 
          difficulty = 0;
        while (analogRead(inY) < 200){}
        lcd.clear();
      }  
    }
    else if (BUTTONPRESSED == ifInput()) {
      return difficulty;
    }
    lcd.setCursor(3,0);
    lcd.print("Difficulty");
    lcd.setCursor(1,1);
    if (difficulty == 0)
      lcd.print("<-   EASY   -> ");
    if (difficulty == 1)
        lcd.print("<-  MEDIUM  -> ");
    if (difficulty == 2)
        lcd.print("<-   HARD   -> ");
    if (difficulty == 3)
        lcd.print("<-  BRUTAL  -> ");
  }
}

//Alustetaan pelin pommienmäärä ja voittoehto riippuen vaikeusaste valinnasta
void selectedDifficulty(int difficulty) {
  if (difficulty == EASY_DIFFICULTY) {
    bombAmount = (MAX_WIDTH*MAX_HEIGHT) / 10;
    victory = (MAX_WIDTH*MAX_HEIGHT)-bombAmount;
  }
  if (difficulty == MEDIUM_DIFFICULTY) {
    bombAmount = (MAX_WIDTH*MAX_HEIGHT) / 6;
    victory = (MAX_WIDTH*MAX_HEIGHT)-bombAmount;
  }
  if (difficulty == HARD_DIFFICULTY) {
    bombAmount = (MAX_WIDTH*MAX_HEIGHT) / 4;
    victory = (MAX_WIDTH*MAX_HEIGHT)-bombAmount;
  }
  if (difficulty == BRUTAL_DIFFICULTY) {
    bombAmount = (MAX_WIDTH*MAX_HEIGHT) / 2;
    victory = (MAX_WIDTH*MAX_HEIGHT)-bombAmount;
  }
}

void playTone(char note,int duration) {
  char notes[] = {'C','D','E','F','G','A','B','e','d'};
  int frequency[] = {261,293,329,349,391,440,493,164,146};
  int index;
  for (int i = 0; i < 7; i++) {
    if (notes[i] == note) index = i;
  }
  tone(speakerPin,frequency[index],duration);
  delay(duration*0.75);
  noTone(speakerPin);
  delay(duration*0.25);
}
