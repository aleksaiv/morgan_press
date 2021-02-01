#include <LiquidCrystal_I2C.h>
#include <RotaryEncoder.h>

#define PIN_ENCODER_1 2
#define PIN_ENCODER_2 3
#define PIN_ENCODER_BTN 4

RotaryEncoder encoder(PIN_ENCODER_1, PIN_ENCODER_2);
LiquidCrystal_I2C lcd(0x27,20,4);

#define MENU_CMD_PRINT 0 
#define MENU_CMD_HEADER -1
#define MENU_CMD_NITEMS -2
#define MENU_CMD_ITEM_TYPE -3
#define MENU_CMD_ENTER -4
#define MENU_CMD_UP -5
#define MENU_CMD_DOWN -6

#define MENU_ITYPE_FIELD 1
#define MENU_ITYPE_SUBMENU 2
#define MENU_ITYPE_CANCEL 3
#define MENU_ITYPE_OK 4


#define NUMBER_OF_TIMERS 7

typedef struct {
  uint16_t v;
  uint16_t m;
  char d[21];
} Timer;

Timer t13 = {2000,1000, "Clamp push"};
Timer tinj = {6000,1000, "Injection"};
Timer t34 = {1000,500, "Cooling"};
Timer trr = {500,500,"Ram return"};
Timer t42 = {1000,500, "Clamp off"};
Timer tcr = {500,2000,"Clamp pull"};
Timer t12 = {1000,500,"T12"};

Timer *timers[NUMBER_OF_TIMERS] = {&t13, &tinj, &t34, &trr, &t42, &tcr, &t12};

char getKey() {
  static long encoder_pos = 0, encoder_oldPos = 0;
  static unsigned long enc_btn_m;
  static boolean enc_btn_old = false;
  boolean enc_btn;
  char k = ' ';
  
  encoder_pos = encoder.getPosition();
  if (encoder_pos != encoder_oldPos) {
    if (encoder_pos > encoder_oldPos)
      k = 'U';
    else
    if (encoder_pos < encoder_oldPos)
      k = 'D';
    encoder_oldPos = encoder_pos;
  }
  enc_btn = !digitalRead(PIN_ENCODER_BTN);
  if (enc_btn != enc_btn_old){
    enc_btn_m = millis();
    enc_btn_old = enc_btn;
  }
  if (enc_btn && millis() - enc_btn_m > 100 && enc_btn_m > 0) {
    k = 'E';
    enc_btn_m = 0;
  }
  return k;
}
void setup()
{
  Serial.begin(9600);
  lcd.init();
  lcd.backlight(); 
  lcd.setCursor(3,0);
  lcd.print("Hello, world!");
  
  // Setup ISR for encoder pins
  PCICR |= 0b00000100;
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);  
} 
ISR(PCINT2_vect) {
  encoder.tick();
}
int menu_handler(int m, int p, int cmd = 0, boolean edit = false) {
  char buf[21];
  switch(m){
    case 0: // MENU MAIN
      switch(cmd){
        case MENU_CMD_HEADER: lcd.print("Main menu"); return 0; 
        case MENU_CMD_NITEMS: return 3;
        case MENU_CMD_ITEM_TYPE: // item type
          switch(p){
            case 0: return MENU_ITYPE_SUBMENU;
            case 1: return MENU_ITYPE_SUBMENU; // submenu
            case 2: return MENU_ITYPE_CANCEL; // Cancel (back)
            default: return 0;
          }
        case MENU_CMD_ENTER:
          switch(p){
            case 0: return 1;
            case 1: return 2;
          }
        case MENU_CMD_PRINT:
          switch(p){
            case 0:
              lcd.print(F("Run SINGLE CYCLE"));
              break;
            case 1:
              lcd.print(F("Configuration"));
              break;
            case 2:
              lcd.print(F("<Back"));
              break;
            default:
              lcd.print(p);
              break;
          }
          break;
      }
      break;
    case 1: // MENU "Run SINGLE CYCLE
      switch(cmd){
        case MENU_CMD_HEADER: lcd.print("Run SINGLE CYCLE"); return 0;
        case MENU_CMD_NITEMS: return 2;
        case MENU_CMD_ITEM_TYPE:
          switch(p){
            case 0: return MENU_ITYPE_OK; // OK (Proceed)
            case 1: return MENU_ITYPE_CANCEL; // Cancel
          }
        case MENU_CMD_ENTER:
          if(p==0)return 1;
          else return 0;
        case MENU_CMD_PRINT:
          switch(p){
            case 0:
              lcd.print(F("Proceed"));
              break;
            case 1:
              lcd.print(F("Cancel"));
              break;
          }
          break;
      }
      break;
    case 2:  // MENU Configuration 
      switch(cmd){
        case MENU_CMD_HEADER:  lcd.print("Configuration"); return 0;
        case MENU_CMD_NITEMS: return NUMBER_OF_TIMERS+4+1;
        case MENU_CMD_ITEM_TYPE:
          if(p<NUMBER_OF_TIMERS+4) return MENU_ITYPE_FIELD;
          if(p == NUMBER_OF_TIMERS+4) return MENU_ITYPE_CANCEL;
          return MENU_ITYPE_FIELD;
          break;
        case MENU_CMD_UP:
        case MENU_CMD_DOWN:
          if(p<NUMBER_OF_TIMERS)
            if(cmd==MENU_CMD_DOWN)
              if(timers[p]->v<59500)timers[p]->v+=500; else timers[p]->v=60000;
            else 
              if(timers[p]->v-500 > timers[p]->m) timers[p]->v -= 500; else timers[p]->v=timers[p]->m;
          break;
        case MENU_CMD_PRINT:
          if(p<NUMBER_OF_TIMERS){
             sprintf(buf,"%-13s%c",timers[p]->d, edit ? ':' : ' ');
             lcd.print(buf);
             dtostrf(timers[p]->v/1000.0,4,1,buf);
             lcd.print(buf);lcd.print("s");
          }else if(p<NUMBER_OF_TIMERS+4){
            switch(p-NUMBER_OF_TIMERS){
              case 0:lcd.print("T1");break;
              case 1:lcd.print("T2");break;
              case 2:lcd.print("CP");break;
              case 3:lcd.print("PP");break;
            }
          }else {
            lcd.print("<Back");
          }
          break;
      }
      break;
  }
  return 0;
}
int menu(int m){
  int pos = 0;
  int oldpos = -1;
  int offset = 0;
  int oldoffset = -1;
  int n;
  boolean edit = false;
  boolean refresh = true;
  int itype;
  int r;
  
  char k;
  n = menu_handler(m, 0, -2);
  while (1) {
    k = getKey();
    if (edit) {
      if (k == 'U' || k == 'D')
      {
        menu_handler(m, offset + pos, k == 'U' ? MENU_CMD_UP : MENU_CMD_DOWN);
        lcd.setCursor(1, pos + 1);
        menu_handler(m, offset + pos, MENU_CMD_PRINT, edit);
        continue;
      }
    }
    if (k == 'E') {
      if (edit){
        edit = false;
        lcd.setCursor(1, pos+1);
        menu_handler(m, offset + pos, MENU_CMD_PRINT, edit);
        continue;
      }
      itype = menu_handler(m, offset+pos, MENU_CMD_ITEM_TYPE); 
      switch(itype) {
        case MENU_ITYPE_CANCEL:
          lcd.clear();
          return 0;
        case MENU_ITYPE_OK:
          r = menu_handler(m, offset+pos, MENU_CMD_ENTER);
          lcd.clear();
          return r;
        case MENU_ITYPE_FIELD:
          edit = true;
          lcd.setCursor(1, pos+1);
          menu_handler(m, offset + pos, MENU_CMD_PRINT, edit);
          break;
        case MENU_ITYPE_SUBMENU:  
          r = menu_handler(m, offset+pos, MENU_CMD_ENTER);
          if (menu(r) == 1)
            return 1;
          refresh = true;
          break;
      }
    }
    if(!edit){
    if(k == 'U'){
      pos--;
      if(pos < 0) {
        if(offset >= 3){
          offset -= 3;
          pos = 2;
        } else
          pos = 0;
      }
    }
    if(k == 'D'){
      pos++;
      if(pos>=3) {
        if(offset + pos < n) {
          offset+=3;
          pos = 0;
        } else 
          pos = 2;
      }
      if (offset + pos >= n)
        pos = n - offset - 1;
    }
    }
    if(offset + pos != oldoffset+oldpos || refresh) {
      oldpos =  pos;
      if(offset != oldoffset || refresh ){
        refresh = false;
        lcd.clear();
        menu_handler(m, 0, MENU_CMD_HEADER);  // print menu header
      }
      oldoffset = offset;
      for(int i = offset; i <  offset + 3 && i < n; i++){
        lcd.setCursor(0, 1 + i - offset);
        if(i == offset + pos)
          lcd.print(">");
        else
          lcd.print(" ");
        menu_handler(m, i, MENU_CMD_PRINT, edit); // print a menu item
      }
    }
  }
  lcd.clear();
}
void loop()
{
  char k;
  k = getKey();
  if(k!=' ')lcd.print(k);
  if(k == 'E')
    menu(0);
} // loop ()

// The End
